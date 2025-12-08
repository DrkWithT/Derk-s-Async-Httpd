#ifndef DERKHTTPD_MYURI_PARSE_HPP
#define DERKHTTPD_MYURI_PARSE_HPP

#include <cstdint>
#include <string_view>
#include <string>
#include <expected>

#include "myuri/uri.hpp"

namespace DerkHttpd::Uri {
    enum class TokenTag : uint8_t {
        unknown,
        path, // alphanumeric string with slashes e.g `/foo/bar`
        wordy, // `(ALPHA | DIGIT | SPECIALS)+` where `SPECIALS = '/' | ':'`
        query_mark, // `'?'`
        query_assign, // `'='`
        query_delim, // `'&'`
        item_int, // `(DIGIT)+`
        item_encoded_char, // `PERCENT (HEX){2}`
        eos,
    };

    struct Token {
        int begin;
        int length;
        TokenTag tag;
        char unescaped; // is `NUL` char if the token is not a percent-encoded character

        constexpr Token(int begin_, int length_, TokenTag tag_, char unescaped_) noexcept
        : begin {begin_}, length {length_}, tag {tag_}, unescaped {unescaped_} {}

        constexpr Token(int begin_, int length_, TokenTag tag_) noexcept
        : Token (begin_, length_, tag_, '\0') {}

        constexpr Token(int begin_, char unescaped_) noexcept
        : Token (begin_, 1, TokenTag::item_encoded_char, unescaped_) {}

        template <std::same_as<TokenTag> FirstTag, typename ... MoreTags>
        [[nodiscard]] constexpr auto match_tag_to(this auto&& self, FirstTag first, MoreTags ... more) noexcept -> bool {
            return ((self.tag == first) | ... | (self.tag == more));
        }

        [[nodiscard]] constexpr auto as_pct_decoded_char(this auto&& self) noexcept -> char {
            return self.unescaped;
        }

        [[nodiscard]] constexpr auto as_string_view(this auto&& self, std::string_view sv) -> std::string_view {
            return sv.substr(self.begin, self.length);
        }
    };

    class Lexer {
    private:
        int m_pos;
        int m_end;

        [[nodiscard]] static constexpr auto match_alpha(char c) noexcept -> bool {
            return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
        }

        [[nodiscard]] static constexpr auto match_digit(char c) noexcept -> bool {
            return (c >= '0' && c <= '9');
        }

        [[nodiscard]] static constexpr auto match_hex(char c) noexcept -> bool {
            return (c >= 'A' && c <= 'F') || (c >= '0' && c <= '9');
        }

        [[nodiscard]] auto at_eos() const noexcept -> bool;
        [[nodiscard]] auto lex_single(TokenTag tag) noexcept -> Token;
        [[nodiscard]] auto lex_encoded_char(std::string_view src_sv) noexcept -> Token;
        [[nodiscard]] auto lex_textual(std::string_view src_sv) noexcept -> Token;
        [[nodiscard]] auto lex_int(std::string_view src_sv) noexcept -> Token;

    public:
        Lexer(std::string_view str_sv) noexcept;

        [[nodiscard]] auto operator()(std::string_view src_sv) noexcept -> Token;
    };

    class Parser {
    private:
        Uri m_result;
        Token m_current;

        /// For unknown token errors.
        void report_error(const Token& culprit);

        /// For more-elaborate parse errors.
        void report_error(const Token& culprit, const std::string& msg);

        [[nodiscard]] auto advance(std::string_view uri_src, Lexer& lexer) noexcept -> Token;

        void consume_any(std::string_view uri_src, Lexer& lexer) noexcept;

        template <TokenTag First, TokenTag ... More>
        [[nodiscard]] constexpr auto consume_of(std::string_view uri_src, Lexer& lexer) {
            if (m_current.match_tag_to(First, More...)) {
                m_current = advance(uri_src, lexer);
            } else {
                report_error(m_current);
            }
        }

        [[nodiscard]] auto at_eos() const noexcept -> bool;

        void clear_result();

        /// NOTE: This helper method throws, but the exception will be caught by the parent `operator()(...)`. Thus, an `std::expected` is not needed here.
        [[nodiscard]] auto parse_relative_uri(std::string_view uri_src, Lexer& lexer) -> Uri;

        [[nodiscard]] auto parse_path(std::string_view uri_src, Lexer& lexer) -> std::string;
        [[nodiscard]] auto parse_query(std::string_view uri_src, Lexer& lexer) -> std::map<std::string, QueryValue>;
        [[nodiscard]] auto parse_query_item(std::string_view uri_src, Lexer& lexer) -> QueryPair;
        [[nodiscard]] auto parse_query_value(std::string_view uri_src, Lexer& lexer) -> QueryValue;

    public:
        Parser();

        [[nodiscard]] auto operator()(std::string_view uri_src, Lexer& lexer) -> std::expected<Uri, std::string>;
    };

    [[nodiscard]] auto parse_simple_uri(std::string_view uri) -> std::expected<Uri, std::string>;
}

#endif
