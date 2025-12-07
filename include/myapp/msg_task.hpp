#ifndef DERKHTTPD_MYAPP_MSG_TASK_HPP
#define DERKHTTPD_MYAPP_MSG_TASK_HPP

#include <concepts>
#include <iostream>
#include <print>

#include "myhttp/intake.hpp"
#include "myhttp/outtake.hpp"

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

        [[nodiscard]] auto operator()(int fd_idx, int fd) -> ResultType {
            auto req_result = m_http_in(fd);

            // 1. Check if request decode was OK. Usually, a bad exchange means the connection's invariants are broken- It must be closed.
            if (!req_result.has_value()) {
                std::println(std::cerr, "MsgExchangeTask ERROR:\n{}", req_result.error());
                return {fd_idx, false};
            }

            const auto& req = req_result.value();
            auto res = Http::Response {
                .body = {},
                .headers = {},
                .http_schema = Http::Schema::http_1_1,
                .http_status = Http::Status::http_ok,
            };

            res.headers.emplace("Server", "derkhttpd/0.0.1");

            // 2. Try handling persistent connections by the client's wishes and HTTP/1.1 defaults.
            if (req.headers.contains("Connection") && req.http_schema == Http::Schema::http_1_1) {
                res.headers.emplace("Connection", req.headers.at("Connection"));
            } else {
                res.headers.emplace("Connection", "close");
            }

            if (req.uri == "/" && req.http_verb == Http::Verb::http_get) {
                res.body = {'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd'};

                res.headers.emplace("Content-Length", "11");
                res.headers.emplace("Content-Type", "text/plain");
            } else {
                res.http_status = Http::Status::http_not_found;

                res.headers.emplace("Content-Length", "0");
                res.headers.emplace("Content-Type", "*/*");
            }

            if (!m_http_out(fd, res)) {
                return {fd_idx, false};
            }

            return {fd_idx, true};
        }
    };
}

#endif