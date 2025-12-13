#include "myapp/routes.hpp"
#include "myapp/response_helpers.hpp"

namespace DerkHttpd::App {
    const auto dud_fallback_handler = []([[maybe_unused]] Http::Request req, const std::map<std::string, Uri::QueryValue>& params) -> Http::Response {
        Http::Response res;
        
        App::StringReply generic_error_msg {"No matching middleware found.", "text/plain"};
        App::ResponseUtils::response_put_all(res, generic_error_msg, Http::Status::http_not_found);

        return res;
    };


    auto compare_host_str(std::string_view incoming, std::string_view host_name, std::string_view host_port) noexcept -> bool {
        // Case 1: If the host has no ':', it's just a special host name.
        if (!incoming.contains(':')) {
            return incoming == host_name;
        }

        // Case 2 (general): check both hostname and port by the full syntax:
        // https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Headers/Host#syntax
        const auto colon_pos = incoming.find_last_of(':');
        std::string_view incoming_host_name = incoming.substr(0, colon_pos);
        std::string_view incoming_port_name = incoming.substr(colon_pos + 1);

        return incoming_host_name == host_name && incoming_port_name == host_port;
    }


    auto Routes::check_host_header(const Http::Request& req) const -> bool {
        if (!req.headers.contains("Host") && req.http_schema == Http::Schema::http_1_1) {
            return false;
        }

        return compare_host_str(
            req.headers.at("Host"),
            m_host_name,
            m_host_port
        );
    }

    Routes::Routes(std::string_view server_host_name, std::string_view server_host_port)
    : m_fallback {dud_fallback_handler}, m_handlers {}, m_host_name {server_host_name}, m_host_port {server_host_port} {}

    auto Routes::set_handler(const std::string& route_path, Middleware handler_box) noexcept -> bool {
        if (m_handlers.contains(route_path)) {
            return false;
        }

        m_handlers.emplace(route_path, handler_box);

        return true;
    }
}
