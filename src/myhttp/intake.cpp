#include <utility>
#include <algorithm>
#include <string>

#include <sstream>
#include <iostream>
#include <print>

#include "myhttp/intake.hpp"

namespace DerkHttpd::Http {
    using std::literals::operator""s;

    constexpr auto http_chunk_prefix_base = 16;
    constexpr auto http_chunk_prefix_dud = -1;

    auto HttpIntake::parse_request_line(std::string_view sv) -> std::expected<RawReqLine, std::string> {
        std::istringstream tokenizer {std::format("{}", sv)};
        std::string verb_lexeme;
        std::string path_lexeme;
        std::string schema_lexeme;

        tokenizer >> verb_lexeme;
        tokenizer >> path_lexeme;
        tokenizer >> schema_lexeme;

        return RawReqLine {
            .rel_uri = std::move(path_lexeme),
            .verb = m_verbs.contains(verb_lexeme) ? m_verbs.at(verb_lexeme) : Verb::http_get,
            .schema = m_schemas.contains(schema_lexeme) ? m_schemas.at(schema_lexeme) : Schema::http_1_1,
        };
    }

    // TODO: skip leading & trailing spaces around header key and value.
    auto HttpIntake::parse_request_header(std::string_view sv) -> std::expected<RawHeader, std::string> {
        if (sv.empty()) {
            return RawHeader {
                .key = {},
                .value = {},
            };
        }

        std::istringstream tokenizer {std::format("{}", sv)};
        std::string header_key_lexeme;
        std::string header_value_lexeme;

        std::getline(tokenizer, header_key_lexeme, ':');

        // Skip extra spaces between ':' and <value> for correct strings.
        tokenizer.ignore(1, ' ');

        std::getline(tokenizer, header_value_lexeme);

        return RawHeader {
            .key = std::move(header_key_lexeme),
            .value = std::move(header_value_lexeme),
        };
    }

    auto HttpIntake::handle_state_request_line(int fd) -> State {
        auto io_result = Net::socket_read_line(fd, m_buffer);

        if (!io_result.has_value()) {
            return State::httpin_state_syntax_error;
        }

        std::string_view temp_line {m_buffer.data(), static_cast<std::size_t>(io_result.value())};

        if (auto request_line = parse_request_line(temp_line); !request_line.has_value()) {
            return State::httpin_state_syntax_error;
        } else {
            auto& [req_path, req_verb, req_schema] = request_line.value();

            m_temp.uri = std::move(req_path);
            m_temp.http_verb = req_verb;
            m_temp.http_schema = req_schema;
        }

        return State::httpin_state_header;
    }

    auto HttpIntake::handle_state_header(int fd) -> State {
        auto io_result = Net::socket_read_line(fd, m_buffer);

        if (!io_result.has_value()) {
            return State::httpin_state_syntax_error;
        }

        std::string_view temp_line {m_buffer.data(), static_cast<std::size_t>(io_result.value())};

        if (temp_line.length() >= static_cast<std::size_t>(m_max_header_size)) {
            return State::httpin_state_constraint_error;
        }

        if (auto request_line = parse_request_header(temp_line); !request_line.has_value()) {
            return State::httpin_state_syntax_error;
        } else if (auto& [key , value] = request_line.value(); !key.empty() && !value.empty()) {
            m_temp.headers[key] = std::move(value);

            return State::httpin_state_header;
        } else {
            return State::httpin_state_choose_body_mode;
        }
    }

    auto HttpIntake::handle_state_choose_body_mode() -> State {
        if (m_temp.headers.contains("Transfer-Encoding")) {
            if (const auto& transfer_encoding = m_temp.headers.at("Transfer-Encoding"); transfer_encoding == "chunked") {
                return State::httpin_state_chunk;
            }
        }

        return State::httpin_state_simple_body;
    }

    auto HttpIntake::handle_state_simple_body(int fd) -> State {
        Blob temp_body;

        auto pending_body_n = (m_temp.headers.contains("Content-Length")) ? std::stoi(m_temp.headers.at("Content-Length")) : 0;

        if (pending_body_n > m_max_body_size) {
            // std::println(std::cerr, "Intake ERR: request body too big!");
            return State::httpin_state_constraint_error;
        }

        while (pending_body_n > 0) {
            if (auto recv_result = Net::socket_read_n(fd, pending_body_n, m_buffer); !recv_result.has_value()) {
                return State::httpin_state_syntax_error;
            } else if (const auto read_n = recv_result.value(); read_n > 0) {
                for (std::string_view fragment_view = m_buffer.data(); const auto ch : fragment_view) {
                    temp_body.emplace_back(ch);
                }

                pending_body_n -= read_n;
                std::ranges::fill(m_buffer, '\0');
            } else {
                break;
            }
        }

        m_temp.body = std::move(temp_body);

        return State::httpin_state_done;
    }

    auto HttpIntake::handle_state_chunk(int fd) -> State {
        // 1. As per HTTP/1.1, read the chunk prefix line before parsing the hexadecimal integer. The I/O must succeed prior.
        if (auto io_result = Net::socket_read_line(fd, m_buffer); !io_result.has_value()) {
            std::println(std::cerr, "Intake ERR [HttpIntake::handle_state_chunk (1)]:\n\tFailed to read chunk prefix line.");
            return State::httpin_state_syntax_error;
        }

        const auto chunk_length = ([](std::string prefix_line) noexcept -> int {
            try {
                return std::stoi(prefix_line, nullptr, http_chunk_prefix_base);
            } catch (const std::invalid_argument& syntax_err) {
                return http_chunk_prefix_dud;
            } catch (const std::out_of_range& repr_err) {
                return http_chunk_prefix_dud;
            }
        })({m_buffer.data()});

        // 2. If the prefix length is valid and positive, read another body chunk.
        if (chunk_length > 0) {
            ssize_t pending_chunk_n = chunk_length;

            while (pending_chunk_n > 0) {
                if (auto chunk_temp_io_res = Net::socket_read_n(fd, pending_chunk_n, m_buffer); !chunk_temp_io_res) {
                    std::println(std::cerr, "Intake ERR [HttpIntake::handle_state_chunk (2)]:\n\tFailed to read chunk blob.");

                    return State::httpin_state_syntax_error;
                } else if (const auto chunk_io_read_n = chunk_temp_io_res.value(); chunk_io_read_n > 0) {
                    m_temp.body.append_range(std::string_view {m_buffer.begin(), m_buffer.begin() + chunk_io_read_n});

                    pending_chunk_n -= chunk_io_read_n;
                } else {
                    std::println(std::cerr, "Intake ERR [HttpIntake::handle_state_chunk (3)]:\n\tFailed to read chunk blob fragment.");

                    return State::httpin_state_syntax_error;
                }
            }
        } else if (chunk_length < 0) {
            // 2.1: Check for invalid chunk lengths... This would be unusable, thus requiring a connection termination.
            std::println(std::cerr, "Intake ERR [HttpIntake::handle_state_chunk (3)]:\n\tInvalid chunk prefix length.");

            return State::httpin_state_syntax_error;
        }

        // 2.2: Consume & skip trailing CRLF after the chunk(s).
        if (auto last_crlf_io_res = Net::socket_read_line(fd, m_buffer); !last_crlf_io_res.has_value()) {
            std::println(std::cerr, "Intake ERR [HttpIntake::handle_state_chunk (4)]:\n\tFailed to read chunk-end line.");
            return State::httpin_state_syntax_error;
        }

        return (chunk_length == 0) ? State::httpin_state_done : State::httpin_state_chunk ;
    }

    HttpIntake::HttpIntake(IntakeConfig config) noexcept
    : m_buffer {}, m_verbs {}, m_schemas {}, m_temp {}, m_state {State::httpin_state_request_line}, m_max_header_size {480}, m_max_body_size {config.max_body_size} {
        std::ranges::fill(m_buffer, 0);

        m_verbs.emplace("GET"s, Verb::http_get);
        m_verbs.emplace("HEAD"s, Verb::http_head);
        m_verbs.emplace("POST"s, Verb::http_post);
        m_verbs.emplace("PUT"s, Verb::http_put);
        m_verbs.emplace("DELETE"s, Verb::http_delete);

        m_schemas.emplace("HTTP/1.0"s, Schema::http_1_0);
        m_schemas.emplace("HTTP/1.1"s, Schema::http_1_1);
    }

    auto HttpIntake::operator()(int fd) -> std::expected<Request, std::string> {
        m_state = State::httpin_state_request_line;

        auto request_malformed = false;
        auto request_bad_sema = false;
        auto request_done = false;

        while (!request_done && !request_malformed && !request_bad_sema) {
            switch (m_state) {
                case State::httpin_state_request_line:
                    m_state = handle_state_request_line(fd);
                    break;
                case State::httpin_state_header:
                    m_state = handle_state_header(fd);
                    break;
                case State::httpin_state_choose_body_mode:
                    m_state = handle_state_choose_body_mode();
                    break;
                case State::httpin_state_simple_body:
                    m_state = handle_state_simple_body(fd);
                    break;
                case State::httpin_state_chunk:
                    m_state = handle_state_chunk(fd);
                    break;
                case State::httpin_state_syntax_error:
                    // std::println("Intake ERR:\nFound syntax error in request!");
                    request_malformed = true;
                    break;
                case State::httpin_state_constraint_error:
                    // std::println("Intake ERR:\nFound semantic error in request!");
                    request_bad_sema = true;
                    break;
                case State::httpin_state_done:
                default:
                    request_done = true;
                    break;
            }
        }

        if (request_malformed) {
            return std::unexpected {"Invalid request syntax!"};
        } else if (request_bad_sema) {
            return std::unexpected {"Invalid request header / body sizing!"};
        }

        return {std::exchange(m_temp, {})};
    }
}
