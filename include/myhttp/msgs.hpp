#ifndef DERK_HTTPD_MYHTTP_MSGS_HPP
#define DERK_HTTPD_MYHTTP_MSGS_HPP

#include <any>
#include <cstddef>
#include <string>
#include <vector>
#include <map>

#include "myhttp/enums.hpp"

namespace DerkHttpd::Http {
    using Blob = std::vector<char>;

    struct Request {
        Blob body;
        std::map<std::string, std::string> headers;
        std::string uri;
        Verb http_verb;
        Schema http_schema;
    };

    struct Response {
        // For avoiding circular dependency: stores any specific `App::ResourceKind`.
        std::any body;
        std::map<std::string, std::string> headers;
        Status http_status;
        Schema http_schema;
    };
}

#endif
