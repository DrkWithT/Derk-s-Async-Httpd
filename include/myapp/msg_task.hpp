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
        enum class ModifyBoundTag : uint8_t {
            none,
            minimum,
            maximum,
        };

        struct ResourceTimeBound {
            std::chrono::seconds time; // Count of seconds since Epoch for resource modification timestamp.
            ModifyBoundTag is_afterward; // Whether the resource's modification timestamp must exceed `ResourceTimeBound::time` to send.
        };

        Http::HttpIntake m_http_in;
        Http::HttpOuttake m_http_out;

        // NOTE: By MDN, the If-Modified-Since applies only for HEAD & GET requests if applicable. For If-Unmodified-Since, it applies only for non-HEAD & non-GET requests if applicable. This helper member function is important for respecting the caching mechanics of HTTP/1.1.
        [[nodiscard]] auto deduce_resource_time_bound(const Http::Request& request) -> ResourceTimeBound {
            const auto request_verb = request.http_verb;

            if (request.headers.contains("If-Modified-Since") && (request_verb == Http::Verb::http_head || request_verb == Http::Verb::http_get)) {
                return {
                    .time = App::parse_date_string(request.headers.at("If-Modified-Since")),
                    .is_afterward = ModifyBoundTag::minimum,
                };
            } else if (request.headers.contains("If-Unmodified-Since") && (request_verb != Http::Verb::http_head && request_verb != Http::Verb::http_get)) {
                return {
                    .time = App::parse_date_string(request.headers.at("If-Unmodified-Since")),
                    .is_afterward = ModifyBoundTag::maximum,
                };
            } else {
                return {
                    .time = App::get_epoch_seconds_now(),
                    .is_afterward = ModifyBoundTag::none,
                };
            }
        }

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
            const auto [resource_modify_time_bound, modify_bound_tag] = deduce_resource_time_bound(req);
            const auto req_is_head = req.http_verb == Http::Verb::http_head;

            // 2a. If applicable, handle HEAD requests as per HTTP/1.1, resembling GET requests without the body.
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

            // 2b. Send a 304 when the resource's timestamp (in Epoch seconds) is below a minimum time or a 412 when the resource's timestamp exceeds the minimum unmodified-since time. See `MsgExchangeTask::deduce_resource_time_bound()`.
            if (std::holds_alternative<std::filesystem::file_time_type>(res.modify_timestamp)) {
                if (const auto& res_resource_timestamp = std::get<std::filesystem::file_time_type>(res.modify_timestamp); modify_bound_tag == ModifyBoundTag::minimum && res_resource_timestamp.time_since_epoch() <= resource_modify_time_bound) {
                    res.body = {};

                    // NOTE: A 304 should not send any payload, so there's no need for resource-payload headers. Only `Last-Modified` should be most important for the client's possible caching.
                    res.headers.clear();
                    res.headers.emplace(
                        "Last-Modified",
                        std::format(
                            "{0:%a}, {0:%e} {0:%b} {0:%Y} {0:%H}:{0:%M}:{0:%S} UTC",
                            std::chrono::time_point_cast<std::chrono::seconds>(res_resource_timestamp)
                        )
                    );

                    res.http_status = Http::Status::http_not_modified;
                } else if (modify_bound_tag == ModifyBoundTag::maximum && res_resource_timestamp.time_since_epoch() > resource_modify_time_bound) {
                    res.body = {};

                    // NOTE: A 412 should not send any payload as well, similarly to the 304 case above.
                    res.headers.clear();
                    res.headers.emplace(
                        "Last-Modified",
                        std::format(
                            "{0:%a}, {0:%e} {0:%b} {0:%Y} {0:%H}:{0:%M}:{0:%S} UTC",
                            std::chrono::time_point_cast<std::chrono::seconds>(res_resource_timestamp)
                        )
                    );

                    res.http_status = Http::Status::http_precondition_failed;
                }
            }

            // 3. Decorate response with other important headers e.g Server, Connection, and Date.
            res.headers.emplace("Server", "derkhttpd/0.1.0");

            if (req.headers.contains("Connection") && req.http_schema == Http::Schema::http_1_1 && res.http_status != Http::Status::http_server_error) {
                res.headers.emplace("Connection", req.headers.at("Connection"));
            } else {
                res.headers.emplace("Connection", "close");
            }

            res.headers.emplace("Date", get_date_string());

            res.http_schema = req.http_schema;

            if (!m_http_out(fd, res)) {
                return {fd_idx, false};
            }

            return {fd_idx, res.headers.at("Connection") != "close"};
        }
    };
}

#endif