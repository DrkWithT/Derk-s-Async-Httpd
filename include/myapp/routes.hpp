#ifndef DERKHTTPD_MYAPP_ROUTES_HPP
#define DERKHTTPD_MYAPP_ROUTES_HPP

#include <algorithm>
#include <map>
#include <functional>
#include <type_traits>

#include "myhttp/msgs.hpp"
#include "myuri/uri.hpp"
#include "myuri/parse.hpp"
#include "myapp/response_helpers.hpp"

namespace DerkHttpd::App {
    /// NOTE: Provides an alias for any callable entity that generates a web response given a path and some parameters.
    using Middleware = std::function<Http::Response(Http::Request, const std::map<std::string, Uri::QueryValue>&)>;

    [[nodiscard]] auto compare_host_str(std::string_view incoming, std::string_view host_name, std::string_view host_port) noexcept -> bool;

    class Routes {
    private:
        Middleware m_fallback;
        std::map<std::string, Middleware> m_handlers;
        std::string_view m_host_name;
        std::string_view m_host_port;

        /// NOTE: checks if the request has a valid "Host" (if HTTP/1.1)
        [[nodiscard]] auto check_host_header(const Http::Request& req) const -> bool;

    public:
        explicit Routes(std::string_view server_host_name, std::string_view server_host_port);

        [[maybe_unused]] auto set_handler(const std::string& route_path, Middleware handler_box) noexcept -> bool;

        template <typename Req> requires (std::is_same_v<std::remove_cvref_t<Req>, Http::Request>)
        [[nodiscard]] auto dispatch_handler(Req&& req) const noexcept -> Http::Response {
            auto req_uri = Uri::parse_simple_uri(req.uri);

            // 1. Validate request syntax & semantics to prevent false-positive responses.
            if (!check_host_header(req) || !req_uri) {
                // Protocol syntax errors: `Host` header is missing or URI is malformed. 
                Http::Response res;
                App::EmptyReply uri_syntax_error {Http::Status::http_bad_request};

                App::ResponseUtils::response_put_all(res, uri_syntax_error);

                return res;
            }

            const auto& uri_obj = req_uri.value();

            // 2. Try finding the matching route handler by URI path.
            if (auto handler_it = std::find_if(m_handlers.begin(), m_handlers.end(), [&uri_obj](const auto& route_handler_item) -> bool {
                return route_handler_item.first == uri_obj.path();
            }); handler_it != m_handlers.end()) {
                return handler_it->second(std::forward<Req>(req), uri_obj.params());
            }

            // 3. Call the default error handler on an unset middleware route. It gives a simple 404 response for now.
            return m_fallback(req, {});
        }
    };
}

#endif
