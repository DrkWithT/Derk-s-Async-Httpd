#include <utility>
#include <algorithm>
#include <variant>

#include "myuri/parse.hpp"
#include "myapp/routes.hpp"

namespace DerkHttpd::App {
    const auto dud_fallback_handler = []([[maybe_unused]] const Http::Request& req, [[maybe_unused]] const std::map<std::string, Uri::QueryValue>& params) -> Http::Response {
        Http::Response res;

        if (auto error_is_not_found_p = std::get_if<int>(&params.at("is_not_found")); error_is_not_found_p) {
            res.http_status = (*error_is_not_found_p != 0)
                ? Http::Status::http_not_found
                : Http::Status::http_server_error;
        }

        if (auto handle_error_msg_p = std::get_if<std::string>(&params.at("err_msg")); handle_error_msg_p) {
            Http::Blob temp_msg_data;
            temp_msg_data.append_range(*handle_error_msg_p);

            const auto msg_length = handle_error_msg_p->length();

            res.headers.emplace("Content-Type", "text/plain");
            res.headers.emplace("Content-Length", std::to_string(msg_length));

            res.body = std::move(temp_msg_data);
        } else {
            res.headers.emplace("Content-Type", "*/*");
            res.headers.emplace("Content-Length", "0");

            res.body = {};
        }

        return res;
    };

    Routes::Routes()
    : m_fallback {dud_fallback_handler}, m_handlers {} {}

    auto Routes::set_handler(const std::string& route_path, Middleware handler_box) noexcept -> bool {
        if (m_handlers.contains(route_path)) {
            return false;
        }

        m_handlers.emplace(route_path, handler_box);

        return true;
    }

    auto Routes::dispatch_handler(const Http::Request& req) const noexcept -> Http::Response {
        std::string handler_err_msg {"Check server log."};
        auto is_error_not_found = false;

        // 1. Try parsing the URI, but any error next would require a fitting HTTP 404 or 500 reply...
        if (auto req_uri = Uri::parse_simple_uri(req.uri); req_uri.has_value()) {
            const auto& uri_obj = req_uri.value();

            // 2. Try finding the exactly matching route handler by URI path...
            if (auto handler_it = std::find_if(m_handlers.begin(), m_handlers.end(), [&uri_obj](const auto& route_handler_item) -> bool {
                return route_handler_item.first == uri_obj.path();
            }); handler_it != m_handlers.end()) {
                return handler_it->second(req, uri_obj.params());
            }

            is_error_not_found = true;
        } else {
            handler_err_msg = std::exchange(req_uri.error(), {});
        }

        // 3. Call the default error handler on invalid URI or unknown handler. This handler MUST check is_not_found to give an HTTP-404 if appropriate.
        return m_fallback(req, {
            {"is_not_found", {is_error_not_found ? 1 : 0}},
            {"err_msg", handler_err_msg},
        });
    }
}
