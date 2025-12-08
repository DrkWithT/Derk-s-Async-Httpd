#ifndef DERKHTTPD_MYAPP_ROUTES_HPP
#define DERKHTTPD_MYAPP_ROUTES_HPP

#include <map>
#include <functional>

#include "myhttp/msgs.hpp"
#include "myuri/uri.hpp"

namespace DerkHttpd::App {
    /// NOTE: Provides an alias for any callable entity that generates a web response given a path and some parameters.
    using Middleware = std::function<Http::Response(const Http::Request&, const std::map<std::string, Uri::QueryValue>&)>;

    class Routes {
    private:
        Middleware m_fallback;
        std::map<std::string, Middleware> m_handlers;

    public:
        explicit Routes();

        [[maybe_unused]] auto set_handler(const std::string& route_path, Middleware handler_box) noexcept -> bool;

        [[nodiscard]] auto dispatch_handler(const Http::Request& req) const noexcept -> Http::Response;
    };
}

#endif
