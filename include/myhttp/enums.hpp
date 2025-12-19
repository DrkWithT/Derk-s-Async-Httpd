#ifndef DERK_HTTPD_MYHTTP_ENUMS_HPP
#define DERK_HTTPD_MYHTTP_ENUMS_HPP

#include <cstdint>
#include <string_view>
#include <type_traits>

namespace DerkHttpd::Http {
    enum class Verb : uint8_t {
        http_get,
        http_head,
        http_post,
        http_put,
        http_delete,
        last,
    };

    enum class Status : uint32_t {
        http_ok,
        http_not_modified,
        http_permanent_redirect,
        http_bad_request,
        http_not_found,
        http_method_not_allowed,
        http_not_acceptable,
        http_length_required,
        http_precondition_failed,
        http_content_too_large,
        http_request_header_fields_too_large,
        http_server_error,
        http_not_implemented,
        last,
    };

    enum class Schema : uint8_t {
        http_1_0,
        http_1_1,
        http_unknown,
        last,
    };

    [[nodiscard]] auto verb_enum_to_name(Verb v) noexcept -> std::string_view;

    [[nodiscard]] auto status_enum_to_name(Status s) noexcept -> std::string_view;

    [[nodiscard]] auto status_enum_to_code(Status s) noexcept -> std::string_view;

    [[nodiscard]] auto schema_enum_to_name(Schema schema) noexcept -> std::string_view;

    template <typename E> requires requires {{E::last};} && std::is_enum_v<E>
    [[nodiscard]] consteval auto scoped_enum_len() noexcept -> std::size_t {
        return static_cast<std::size_t>(E::last);
    }
}

#endif
