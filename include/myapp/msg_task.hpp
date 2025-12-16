#ifndef DERKHTTPD_MYAPP_MSG_TASK_HPP
#define DERKHTTPD_MYAPP_MSG_TASK_HPP

#include <concepts>
#include <chrono>
#include <iostream>
#include <print>

#include "myhttp/intake.hpp"
#include "myhttp/outtake.hpp"
#include "myapp/routes.hpp"

namespace DerkHttpd::App {
    template <typename ResultType>
    concept TaskResultKind = requires (ResultType result) {
        {auto(result.pollfd_idx)} -> std::same_as<int>;
        {auto(result.ok)} -> std::same_as<bool>;
    };

    /**
     * @brief The callable object meant for the async promises within mynet/handles.cpp... Its logic should handle a request and response I/O exchange between server and client.
     */
    template <TaskResultKind ResultType>
    class MsgExchangeTask {
    private:
        Http::HttpIntake m_http_in;
        Http::HttpOuttake m_http_out;

    public:
        MsgExchangeTask()
        : m_http_in { Http::IntakeConfig {.max_body_size = 1024} }, m_http_out {} {}

        [[nodiscard]] auto operator()(int fd_idx, int fd, const App::Routes& routes) -> ResultType {
            auto req_result = m_http_in(fd);

            // 1. Check if request decode was OK. Usually, a bad exchange means the connection's invariants are broken- It must be closed.
            if (!req_result.has_value()) {
                std::println(std::cerr, "MsgExchangeTask ERROR:\n{}", req_result.error());
                return {fd_idx, false};
            }

            Http::Request req = std::exchange(req_result.value(), {});
            const auto req_is_head = req.http_verb == Http::Verb::http_head;
            const auto req_is_get = req.http_verb == Http::Verb::http_get;

            // 2a. By MDN, the If-Modified-Since header applies only for HEAD & GET requests. Check this as needed!
            const auto req_resource_must_time_check = req.headers.contains("If-Modified-Since") && (req_is_head || req_is_get);

            std::chrono::seconds resource_cache_epoch_s = (req_resource_must_time_check)
                ? App::parse_date_string(req.headers.at("If-Modified-Since"))
                : App::get_epoch_seconds_now();

            // 2b. If applicable, handle HEAD requests as per HTTP/1.1, resembling GET requests without the body.
            if (req_is_head) {
                req.http_verb = Http::Verb::http_get;
            }

            Http::Response res = routes.dispatch_handler(req);

            if (req_is_head) {
                if (auto discardable_chunked_p = std::get_if<App::ChunkIterPtr>(&res.body); discardable_chunked_p) {
                    discardable_chunked_p->get()->clear();
                } else {
                    res.body = {};
                }
            }
            
            // 2c. Send the response's resource only if the response-resource's timestamp (in Epoch seconds) passes the request's `If-Modified-Since` value. Note that `Last-Modified` must exist and have the resource's modification date on a 304.
            if (req_resource_must_time_check && std::holds_alternative<std::filesystem::file_time_type>(res.modify_timestamp)) {
                if (const auto& res_resource_timestamp = std::get<std::filesystem::file_time_type>(res.modify_timestamp); res_resource_timestamp.time_since_epoch() > resource_cache_epoch_s) {
                    res.body = {};
                    res.headers.emplace(
                        "Last-Modified",
                        std::format(
                            "{0:%a}, {0:%e} {0:%b} {0:%Y} {0:%H}:{0:%M}:{0:%S} UTC",
                            std::chrono::time_point_cast<std::chrono::seconds>(res_resource_timestamp)
                        )
                    );
                    res.headers.emplace("Content-Length", "0");
                    res.http_status = Http::Status::http_not_modified;
                }
            }

            // 3. Decorate response with transfer-specific headers e.g Server, Connection, etc.
            res.headers.emplace("Server", "derkhttpd/0.1.0");

            if (req.headers.contains("Connection") && req.http_schema == Http::Schema::http_1_1 && res.http_status != Http::Status::http_server_error) {
                res.headers.emplace("Connection", req.headers.at("Connection"));
            } else {
                res.headers.emplace("Connection", "close");
            }

            res.http_schema = req.http_schema;

            if (!m_http_out(fd, res)) {
                return {fd_idx, false};
            }

            return {fd_idx, res.headers.at("Connection") != "close"};
        }
    };
}

#endif