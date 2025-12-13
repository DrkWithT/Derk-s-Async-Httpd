#include <utility>

#include "myapp/routes.hpp"
#include "myapp/response_helpers.hpp"

namespace DerkHttpd::App {
    const auto dud_fallback_handler = []([[maybe_unused]] Http::Request req, const std::map<std::string, Uri::QueryValue>& params) -> Http::Response {
        Http::Response res;
        
        App::StringReply generic_error_msg {"No matching middleware found.", "text/plain"};
        App::ResponseUtils::response_put_all(res, generic_error_msg, Http::Status::http_not_found);

        return res;
    };

    auto Routes::check_host_header(const Http::Request& req) const -> bool {
        if (req.http_schema == Http::Schema::http_1_1 && !req.headers.contains("Host")) {
            return false;
        } else if (const auto& host_value = req.headers.at("Host"); host_value != m_host_str) {
            return false;
        }

        return true;
    }

    Routes::Routes(std::string server_host)
    : m_fallback {dud_fallback_handler}, m_handlers {}, m_host_str {std::move(server_host)} {}

    auto Routes::set_handler(const std::string& route_path, Middleware handler_box) noexcept -> bool {
        if (m_handlers.contains(route_path)) {
            return false;
        }

        m_handlers.emplace(route_path, handler_box);

        return true;
    }
}
