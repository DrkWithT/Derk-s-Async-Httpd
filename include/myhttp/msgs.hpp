#ifndef DERK_HTTPD_MYHTTP_MSGS_HPP
#define DERK_HTTPD_MYHTTP_MSGS_HPP

#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <variant>

#include "myhttp/enums.hpp"

namespace DerkHttpd::Http {
    using Blob = std::vector<char>;
}

namespace DerkHttpd::App {
    /// TODO: make generic to handle both binary and human-readable content.
    class ChunkIterBase {
    public:
        virtual ~ChunkIterBase() = default;

        virtual auto next() -> std::optional<Http::Blob> = 0;

        virtual void clear() = 0;
    };

    /// NOTE: see `ChunkIterBase` for TODOs.
    using ChunkIterPtr = std::shared_ptr<ChunkIterBase>;
}

namespace DerkHttpd::Http {
    struct Request {
        Blob body;
        std::map<std::string, std::string> headers;
        std::string uri;
        Verb http_verb;
        Schema http_schema;
    };

    struct Response {
        // For avoiding circular dependency: stores any specific `App::ResourceKind`.
        std::variant<Blob, App::ChunkIterPtr> body;
        std::map<std::string, std::string> headers;
        Status http_status;
        Schema http_schema;
    };
}

#endif
