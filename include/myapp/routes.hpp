#ifndef DERKHTTPD_MYAPP_ROUTES_HPP
#define DERKHTTPD_MYAPP_ROUTES_HPP

#include <algorithm>
#include <map>
#include <functional>
#include <type_traits>

#include "myhttp/msgs.hpp"
#include "myuri/uri.hpp"
#include "myuri/parse.hpp"

namespace DerkHttpd::App {
    /// NOTE: Provides an alias for any callable entity that generates a web response given a path and some parameters.
    using Middleware = std::function<Http::Response(Http::Request, const std::map<std::string, Uri::QueryValue>&)>;

    class Routes {
    private:
        Middleware m_fallback;
        std::map<std::string, Middleware> m_handlers;

    public:
        explicit Routes();

        [[maybe_unused]] auto set_handler(const std::string& route_path, Middleware handler_box) noexcept -> bool;

        template <typename Req> requires (std::is_same_v<std::remove_cvref_t<Req>, Http::Request>)
        [[nodiscard]] auto dispatch_handler(Req&& req) const noexcept -> Http::Response {
            std::string handler_err_msg {"Check server log."};
            auto is_error_not_found = false;

            // 1. Try parsing the URI, but any error next would require a fitting HTTP 404 or 500 reply...
            if (auto req_uri = Uri::parse_simple_uri(req.uri); req_uri.has_value()) {
                const auto& uri_obj = req_uri.value();

                // 2. Try finding the exactly matching route handler by URI path...
                if (auto handler_it = std::find_if(m_handlers.begin(), m_handlers.end(), [&uri_obj](const auto& route_handler_item) -> bool {
                    return route_handler_item.first == uri_obj.path();
                }); handler_it != m_handlers.end()) {
                    return handler_it->second(std::forward<Req>(req), uri_obj.params());
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
    };
}

#endif
