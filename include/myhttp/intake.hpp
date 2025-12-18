#ifndef DERK_HTTPD_MYHTTP_INTAKE_HPP
#define DERK_HTTPD_MYHTTP_INTAKE_HPP

#include <expected>
#include <format>
#include <string>
#include <unordered_map>

#include "mynet/io_funcs.hpp"
#include "myhttp/msgs.hpp"

namespace DerkHttpd::Http {
    struct IntakeConfig {
        int max_body_size = 2048;
        // bool report_errors = true;
    };

    enum class TokenTag : uint16_t {
        spaces, // SP, TAB, CR, LF
        identifier,
        verb, // words matching GET, HEAD, POST, PUT, DELETE
        path, // raw URIs
        schema, // raw text of form `'HTTP' '/' <digit> '.' <digit>`
        colon, // ':'
        comma, // ','
        query, // '?'
        ampersand, // '&'
        assign, // '='
        eos, // end of given string
        unknown,
    };

    struct HttpToken {
        int start;
        int length;
        TokenTag tag;
    };

    class HttpIntake {
    private:
        enum class State : unsigned int {
            httpin_state_request_line,
            httpin_state_header,
            httpin_state_choose_body_mode,
            httpin_state_simple_body,
            httpin_state_chunk,
            httpin_state_syntax_error,
            httpin_state_constraint_error,
            httpin_state_done,
        };

        struct RawReqLine {
            std::string rel_uri;
            Verb verb;
            Schema schema;
        };

        struct RawHeader {
            std::string key;
            std::string value;
        };

        Net::ByteBuffer<> m_buffer;
        std::unordered_map<std::string, Verb, std::hash<std::string>> m_verbs;
        std::unordered_map<std::string, Schema, std::hash<std::string>> m_schemas;
        Request m_temp;
        State m_state;
        int m_max_header_size;
        int m_max_body_size;

        [[nodiscard]] auto parse_request_line(std::string_view sv) -> std::expected<RawReqLine, std::string>;
        [[nodiscard]] auto parse_request_header(std::string_view sv) -> std::expected<RawHeader, std::string>;

        [[nodiscard]] auto handle_state_request_line(int fd) -> State;
        [[nodiscard]] auto handle_state_header(int fd) -> State;
        [[nodiscard]] auto handle_state_choose_body_mode() -> State;
        [[nodiscard]] auto handle_state_simple_body(int fd) -> State;
        [[nodiscard]] auto handle_state_chunk(int fd) -> State;

    public:
        explicit HttpIntake(IntakeConfig config) noexcept;

        [[nodiscard]] auto operator()(int fd) -> std::expected<Request, std::string>;
    };
}

#endif
