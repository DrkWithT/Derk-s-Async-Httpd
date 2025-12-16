#include <algorithm>
#include <format>
#include <string>

#include "mynet/io_funcs.hpp"
#include "myhttp/enums.hpp"
#include "myhttp/msgs.hpp"
#include "myhttp/outtake.hpp"

namespace DerkHttpd::Http {
    void HttpOuttake::reset() noexcept {
        std::ranges::fill(m_reply_bytes, '\0');
        m_load_count = 0;
    }

    auto HttpOuttake::serialize(std::string_view sv) noexcept -> bool {
        if (const auto sv_len = sv.length(); m_reply_bytes.size() - m_load_count >= sv_len) {
            std::copy(sv.begin(), sv.end(), m_reply_bytes.begin() + m_load_count);
            m_load_count += sv_len;

            return true;
        }

        return false;
    }

    auto HttpOuttake::write_status_line(int fd, Schema schema, Status status) -> Net::IOResult<ssize_t> {
        reset();

        const auto schema_name_sv = schema_enum_to_name(schema);
        const auto status_code_sv = status_enum_to_code(status);
        const auto status_name_sv = status_enum_to_name(status);

        if (!serialize(schema_name_sv)) {
            return std::unexpected {"Buffer space exhausted for HTTP schema of reply."};
        }

        if (!serialize(" ")) {
            return std::unexpected {"Buffer space exhausted for post-reply-schema SP."};
        }

        if (!serialize(status_code_sv)) {
            return std::unexpected {"Buffer space exhausted for HTTP status code of reply."};
        }

        if (!serialize(" ")) {
            return std::unexpected {"Buffer space exhausted for post-status-code SP."};
        }

        if (!serialize(status_name_sv)) {
            return std::unexpected {"Buffer space exhausted for status-msg of reply."};
        }

        if (!serialize("\r\n")) {
            return std::unexpected {"Buffer space exhausted for CRLF of reply-status-line."};
        }

        return Net::socket_write_n(fd, m_load_count, m_reply_bytes);
    }

    auto HttpOuttake::write_batched_headers(int fd, const std::map<std::string, std::string>& headers) -> Net::IOResult<ssize_t> {
        reset();

        auto has_serialize_err = false;

        for (std::size_t header_count = 0; const auto& [header_key, header_value] : headers) {
            ++header_count;

            if (!serialize(header_key)) {
                has_serialize_err = true;
                break;
            }

            if (!serialize(": ")) {
                has_serialize_err = true;
                break;
            }
            
            if (!serialize(header_value)) {
                has_serialize_err = true;
                break;
            }

            if (!serialize("\r\n")) {
                has_serialize_err = true;
                break;
            }

            if (header_count >= headers.size()) {
                if (!serialize("\r\n")) {
                    has_serialize_err = true;
                    break;
                }
            }

            if (auto io_res = Net::socket_write_n(fd, m_load_count, m_reply_bytes); !io_res) {
                // Report errors ASAP when some I/O goes wrong.
                return io_res;
            } else {   
                reset();
            }
        }

        if (has_serialize_err) {
            return std::unexpected {"Failed to encode a server-made header: exceeds buffer size."};
        }

        return {static_cast<ssize_t>(m_load_count)};
    }

    auto HttpOuttake::write_body(int fd, const Blob& blob) -> Net::IOResult<ssize_t> {
        auto blob_data = blob.data();

        const int output_buffer_size = m_reply_bytes.size();
        int pending_load_n = blob.size();
        int temp_n = 0;
        int done_n = 0;

        std::ranges::fill(m_reply_bytes, '\0');

        while (pending_load_n > 0) {
            temp_n = std::min(pending_load_n, output_buffer_size);

            std::copy_n(blob_data + done_n, temp_n, m_reply_bytes.data());

            if (auto io_result = Net::socket_write_n(fd, temp_n, m_reply_bytes); !io_result) {
                return io_result;
            }

            done_n += temp_n;
            pending_load_n -= temp_n;
        }

        return {static_cast<ssize_t>(done_n)};
    }

    auto HttpOuttake::write_body(int fd, App::ChunkIterPtr chunking_it) -> Net::IOResult<ssize_t> {
        reset();

        Http::Blob http_chunk;

        // NOTE: counts only resource bytes, not including the hex length prefix per chunk!
        ssize_t total_write_count = 0;

        while (true) {
            if (auto next_chunk = chunking_it->next(); !next_chunk) {
                return std::unexpected {"Failed to transmit a file chunk."};
            } else {
                const auto& chunk_payload_blob = next_chunk.value();

                if (!chunk_payload_blob.empty()) {
                    http_chunk.append_range(
                        std::format("{:x}\r\n{}\r\n", chunk_payload_blob.size(), std::string_view {chunk_payload_blob.begin(), chunk_payload_blob.end()})
                    );
                } else {
                    http_chunk.append_range(std::string_view {"0\r\n\r\n"});
                }

                if (auto chunk_io_res = write_body(fd, http_chunk); !chunk_io_res) {
                    return chunk_io_res;
                } else {
                    total_write_count += chunk_io_res.value();
                }

                if (chunk_payload_blob.empty()) {
                    break;
                }

                http_chunk.clear();
            }
        }

        return {total_write_count};
    }

    HttpOuttake::HttpOuttake() noexcept 
    : m_reply_bytes {}, m_load_count {0} {}

    auto HttpOuttake::operator()(int fd, const Response& res) -> bool {
        const auto& [res_body, res_headers, res_time, res_status, res_schema] = res;

        if (auto start_line_io_res = write_status_line(fd, res_schema, res_status); !start_line_io_res) {
            return false;
        }

        if (auto headers_io_res = write_batched_headers(fd, res_headers); !headers_io_res) {
            return false;
        }

        Net::IOResult<ssize_t> body_send_res;

        if (auto blob_p = std::get_if<Http::Blob>(&res.body); blob_p) {
            body_send_res = write_body(fd, *blob_p);
        } else {
            body_send_res = write_body(fd, std::get<App::ChunkIterPtr>(res.body));
        }

        return body_send_res.has_value() && body_send_res.value() > 0;
    }
}
