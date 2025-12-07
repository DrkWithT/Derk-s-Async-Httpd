#include <array>
#include <string_view>

#include "myhttp/enums.hpp"

namespace DerkHttpd::Http {
    constexpr std::array<std::string_view, scoped_enum_len<Verb>()> verb_names {
        "GET",
        "HEAD",
        "POST",
        "PUT",
        "DELETE",
    };

    constexpr std::array<std::string_view, scoped_enum_len<Status>()> status_names {
        "OK",
        "Not Modified",
        "Permanent Redirect",
        "Bad Request",
        "Not Found",
        "Method Not Allowed",
        "Not Acceptable",
        "Length Required",
        "Content Too Large",
        "Request Header Fields Too Large",
        "Internal Server Error",
        "Not Implemented",
    };

    constexpr std::array<std::string_view, scoped_enum_len<Status>()> status_code_names {
        "200",
        "304",
        "308",
        "400",
        "404",
        "405",
        "406",
        "411",
        "413",
        "431",
        "500",
        "501",
    };

    constexpr std::array<std::string_view, scoped_enum_len<Schema>()> schema_names {
        "HTTP/1.0",
        "HTTP/1.1",
        "HTTP/0.0",
    };

    auto verb_enum_to_name(Verb v) noexcept -> std::string_view {
        return verb_names[static_cast<std::size_t>(v)];
    }

    auto status_enum_to_name(Status s) noexcept -> std::string_view {
        return status_names[static_cast<std::size_t>(s)];
    }

    auto status_enum_to_code(Status s) noexcept -> std::string_view {
        return status_code_names[static_cast<std::size_t>(s)];
    }

    auto schema_enum_to_name(Schema schema) noexcept -> std::string_view {
        return schema_names[static_cast<std::size_t>(schema)];
    }
}