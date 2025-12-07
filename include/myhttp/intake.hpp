#ifndef DERK_HTTPD_MYHTTP_INTAKE_HPP
#define DERK_HTTPD_MYHTTP_INTAKE_HPP

#include <expected>
#include <format>
#include <set>
#include <string>
#include <type_traits>
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

    class HttpLexer {
    private:
        std::set<std::string_view> m_verbs; // NOTE: must use const char* content with the equivalent `'static` Rust lifetimes
        std::set<std::string_view> m_schemas; // NOTE: must use const char* content with the equivalent `'static` Rust lifetimes
        std::string_view m_src;
        int m_pos;
        int m_end;

        [[nodiscard]] static constexpr auto match_spaces(char c) noexcept -> bool {
            return c == ' ' || c == '\t' || c == '\r' || c == '\n';
        }

        template <std::same_as<char> FirstChar, typename ... RestChars>
        [[nodiscard]] static constexpr auto unmatched_of(char c, FirstChar first_c, RestChars ... rest_c) noexcept -> bool {
            return ((c != first_c) || ... || (c != rest_c));
        }

        [[nodiscard]] static constexpr auto match_wordy(char c) noexcept -> bool {
            return !match_spaces(c) && unmatched_of(c, ':');
        }

        [[nodiscard]] auto at_eos() const noexcept -> bool;
        [[nodiscard]] auto lex_single(TokenTag tag) -> HttpToken;
        [[nodiscard]] auto lex_spaces() noexcept -> HttpToken;
        [[nodiscard]] auto lex_word() -> HttpToken;

    public:
        HttpLexer(std::string_view sv) noexcept;

        void use_source(std::string_view next_sv) noexcept;

        [[nodiscard]] auto operator()() -> HttpToken;
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
        HttpLexer m_lexer;
        std::unordered_map<std::string, Verb> m_verbs;
        std::unordered_map<std::string, Schema> m_schemas;
        Request m_temp;
        HttpToken m_current;
        State m_state;
        int m_max_header_size;
        int m_max_body_size;

        [[nodiscard]] auto parse_advance() -> HttpToken;
        void parse_consume();

        [[nodiscard]] auto match_token_of(TokenTag tag) const noexcept -> bool;

        [[nodiscard]] static auto stringify_token(std::string_view sv, const HttpToken& token) -> std::string;

        template <typename FirstTag, typename ... Rest> requires (std::is_same_v<FirstTag, TokenTag>)
        void parse_consume_of(FirstTag tag, Rest ... other_tags) {
            if (const auto curr_tag = m_current.tag; ((curr_tag == tag) || ... || (curr_tag == other_tags))) {
                m_current = parse_advance();
                return;
            }

            throw std::runtime_error {std::format("Invalid token of HTTP/1.x message at [ln ?, pos {}] of type {}\n", m_current.start, static_cast<int>(m_current.tag))};
        }

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
