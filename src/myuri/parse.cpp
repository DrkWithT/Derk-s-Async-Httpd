#include <sstream>
#include <utility>
#include <format>
#include <stdexcept>

#include "myuri/parse.hpp"

namespace DerkHttpd::Uri {
    auto Lexer::at_eos() const noexcept -> bool {
        return m_pos >= m_end;
    }

    auto Lexer::lex_single(TokenTag tag) noexcept -> Token {
        const auto temp_begin = m_pos;

        ++m_pos;

        return {temp_begin, 1, tag};
    }

    auto Lexer::lex_encoded_char(std::string_view sv) noexcept -> Token {
        const auto temp_begin = m_pos;
        auto hex_high = '\0';
        auto hex_low = '\0';

        ++m_pos; // skip '%', assuming that the pre-check found it as a prefix!

        if (const auto c0 = sv[m_pos]; !match_hex(c0)) {
            return {temp_begin, 3, TokenTag::unknown};
        } else {
            hex_high = c0;
            ++m_pos;
        }

        if (const auto c1 = sv[m_pos]; !match_hex(c1)) {
            return {temp_begin, 3, TokenTag::unknown};
        } else {
            hex_low = c1;
            ++m_pos;
        }

        int ascii_code = (static_cast<int>(hex_high) << 4) + static_cast<int>(hex_low);

        return {temp_begin, static_cast<char>(ascii_code)};
    }

    auto Lexer::lex_textual(std::string_view src_sv) noexcept -> Token {
        const auto temp_begin = m_pos;
        auto temp_len = 0;
        auto slashes = 0;

        while (!at_eos()) {
            if (const auto c = src_sv[m_pos]; match_alpha(c) || match_digit(c) || c == '.') {
                ++temp_len;
                ++m_pos;
            } else if (c == '/') {
                ++temp_len;
                ++slashes;
                ++m_pos;
            } else {
                break;
            }
        }

        const auto temp_tag = (slashes < 1)
            ? TokenTag::wordy
            : TokenTag::path;

        return {temp_begin, temp_len, temp_tag};
    }

    auto Lexer::lex_int(std::string_view src_sv) noexcept -> Token {
        const auto temp_begin = m_pos;
        auto temp_len = 0;

        while (!at_eos()) {
            if (const auto c = src_sv[m_pos]; match_digit(c)) {
                ++temp_len;
                ++m_pos;
            } else {
                break;
            }
        }

        return {temp_begin, temp_len, TokenTag::item_int};
    }

    Lexer::Lexer(std::string_view str_sv) noexcept
    : m_pos (0), m_end (str_sv.length()) {}

    auto Lexer::operator()(std::string_view src_sv) noexcept -> Token {
        if (at_eos()) {
            return {m_end, 1, TokenTag::eos};
        }

        switch (const auto peeked = src_sv[m_pos]; peeked) {
        case '?': return lex_single(TokenTag::query_mark);
        case '=': return lex_single(TokenTag::query_assign);
        case '&': return lex_single(TokenTag::query_delim);
        case '%': return lex_encoded_char(src_sv);
        default:
            if (peeked == '/' || peeked == '.' || match_alpha(peeked)) {
                return lex_textual(src_sv);
            } else if (match_digit(peeked)) {
                return lex_int(src_sv);
            } else {   
                return lex_single(TokenTag::unknown);
            }
        }
    }


    void Parser::report_error(const Token& culprit) {
        throw std::runtime_error {std::format("URI ERR: Syntax Error at [pos {}]:\nUnknown token!", culprit.begin)};
    }

    void Parser::report_error(const Token& culprit, const std::string& msg) {
        throw std::runtime_error {std::format("URI ERR: Syntax Error at [pos {}]:\n{}", culprit.begin, msg)};
    }

    auto Parser::advance(std::string_view uri_src, Lexer& lexer) noexcept -> Token {
        return lexer(uri_src);
    }

    void Parser::consume_any(std::string_view uri_src, Lexer& lexer) noexcept {
        m_current = advance(uri_src, lexer);
    }

    auto Parser::at_eos() const noexcept -> bool {
        return m_current.tag == TokenTag::eos;
    }

    void Parser::clear_result() {
        m_result = {};
    }

    auto Parser::parse_relative_uri(std::string_view uri_src, Lexer& lexer) -> Uri {
        auto path = parse_path(uri_src, lexer);

        if (!m_current.match_tag_to(TokenTag::query_mark)) {
            return Uri {std::move(path), {}};
        }

        consume_of<TokenTag::query_mark>(uri_src, lexer);

        auto path_params = parse_query(uri_src, lexer);

        return Uri {std::move(path), std::move(path_params)};
    }

    auto Parser::parse_path(std::string_view uri_src, Lexer& lexer) -> std::string {
        std::ostringstream sout;

        while (!at_eos()) {
            if (const auto current_tag = m_current.tag; current_tag == TokenTag::path) {
                sout << std::format("{}", m_current.as_string_view(uri_src));
                consume_any(uri_src, lexer);
            } else if (current_tag == TokenTag::item_encoded_char) {
                sout << m_current.unescaped;
                consume_any(uri_src, lexer);
            } else {
                break;
            }
        }

        return sout.str();
    }

    auto Parser::parse_query(std::string_view uri_src, Lexer& lexer) -> std::map<std::string, QueryValue> {
        std::map<std::string, QueryValue> params;

        while (!at_eos()) {
            auto [item_name, item_value] = parse_query_item(uri_src, lexer);

            params.emplace(std::move(item_name), std::move(item_value));

            if (!m_current.match_tag_to(TokenTag::query_delim)) {
                continue;
            }

            consume_any(uri_src, lexer);
        }

        return params;
    }

    auto Parser::parse_query_item(std::string_view uri_src, Lexer& lexer) -> QueryPair {
        std::string item_name = std::format("{}", m_current.as_string_view(uri_src));
        consume_of<TokenTag::wordy>(uri_src, lexer);

        consume_of<TokenTag::query_assign>(uri_src, lexer);

        auto item_value = parse_query_value(uri_src, lexer);

        return {
            .name = std::move(item_name),
            .value = std::move(item_value),
        };
    }

    auto Parser::parse_query_value(std::string_view uri_src, Lexer& lexer) -> QueryValue {
        QueryValue val;
        std::string lexeme = std::format("{}", m_current.as_string_view(uri_src));

        if (const auto current_tag = m_current.tag; current_tag == TokenTag::item_int) {
            val = std::stoi(lexeme);
        } else if (current_tag == TokenTag::wordy) {
            val = std::move(lexeme);
        }

        consume_of<TokenTag::item_int, TokenTag::wordy>(uri_src, lexer);

        return val;
    }

    Parser::Parser()
    : m_result {}, m_current {0, 0, TokenTag::unknown} {}

    auto Parser::operator()(std::string_view uri_src, Lexer& lexer) -> std::expected<Uri, std::string> {
        consume_any(uri_src, lexer);

        try {
            return parse_relative_uri(uri_src, lexer);
        } catch (const std::runtime_error& parse_err) {
            return std::unexpected {parse_err.what()};
        }
    }


    /// NOTE: This is the only intended helper function to parse relative URIs, using the current lexer & parser.
    auto parse_simple_uri(std::string_view uri) -> std::expected<Uri, std::string> {
        Lexer tokenizer {uri};
        Parser parser;

        return parser(uri, tokenizer);
    }
}
