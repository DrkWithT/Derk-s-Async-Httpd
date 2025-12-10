#ifndef DERK_HTTPD_MYHTTP_OUTTAKE_HPP
#define DERK_HTTPD_MYHTTP_OUTTAKE_HPP

#include "mynet/io_funcs.hpp"
#include "myhttp/msgs.hpp"
#include "myapp/contents.hpp"

namespace DerkHttpd::Http {
    class HttpOuttake {
    private:
        enum class SerializeState : uint8_t {
            srz_name,
            srz_delim,
            srz_value,
            srz_crlf,
            srz_err,
        };

        static constexpr std::size_t normal_flush_bound = 256;
        Net::ByteBuffer<> m_reply_bytes;
        std::size_t m_load_count;

        void reset() noexcept;

        auto serialize(std::string_view sv) noexcept -> bool;

        [[nodiscard]] auto write_status_line(int fd, Schema schema, Status status) -> Net::IOResult<ssize_t>;

        [[nodiscard]] auto write_batched_headers(int fd, const std::map<std::string, std::string>& headers) -> Net::IOResult<ssize_t>;

        [[nodiscard]] auto write_body(int fd, const Blob& blob) -> Net::IOResult<ssize_t>;

        [[nodiscard]] auto write_body(int fd, App::ChunkIterPtr chunking_it) -> Net::IOResult<ssize_t>;

    public:
        HttpOuttake() noexcept;

        [[nodiscard]] auto operator()(int fd, const Response& res) -> bool;
    };
}

#endif
