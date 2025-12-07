#ifndef DERK_HTTPD_MYNET_IO_FUNCS_HPP
#define DERK_HTTPD_MYNET_IO_FUNCS_HPP

#include <expected>
#include <array>
#include <string>

namespace DerkHttpd::Net {
    template <typename Data>
    using IOResult = std::expected<Data, std::string>;

    template <std::size_t Capacity = 512>
    using ByteBuffer = std::array<char, Capacity>;

    [[nodiscard]] auto socket_read_n(int fd, ssize_t n, ByteBuffer<>& dest) -> IOResult<ssize_t>;

    [[nodiscard]] auto socket_read_line(int fd, ByteBuffer<>& dest) -> IOResult<ssize_t>;

    [[nodiscard]] auto socket_write_n(int fd, ssize_t n, const ByteBuffer<>& src) noexcept -> IOResult<ssize_t>;
}

#endif
