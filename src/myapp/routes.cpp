#include <utility>
#include <variant>

#include "myapp/routes.hpp"

namespace DerkHttpd::App {
    const auto dud_fallback_handler = []([[maybe_unused]] Http::Request req, const std::map<std::string, Uri::QueryValue>& params) -> Http::Response {
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
}
