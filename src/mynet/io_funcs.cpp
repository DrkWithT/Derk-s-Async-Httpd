#include <sys/socket.h>
#include <algorithm>

#include "mynet/io_funcs.hpp"

namespace DerkHttpd::Net {
    auto socket_read_n(int fd, ssize_t n, ByteBuffer<>& dest) -> IOResult<ssize_t> {
        std::ranges::fill(dest, '\0');

        auto dest_data_p = dest.data();
        ssize_t pending_rc = std::min(n, ssize_t {512});
        ssize_t done_rc = 0;

        while (pending_rc > 0 && done_rc < n) {
            if (const ssize_t temp_rc = recv(fd, dest_data_p + done_rc, pending_rc, 0); temp_rc > 0) {
                done_rc += temp_rc;
                pending_rc -= temp_rc;
            } else if (temp_rc == 0) {
                return {0};
            } else {
                return std::unexpected {"Invalid read with fd in io_funcs.cpp::socket_read_n(): temp_rc < 0"};
            }
        }

        return {done_rc};
    }

    auto socket_read_line(int fd, ByteBuffer<>& dest) -> IOResult<ssize_t> {
        constexpr auto nul_v = '\0';
        constexpr auto cr_v = '\r';
        constexpr auto lf_v = '\n';

        std::ranges::fill(dest, '\0');

        auto dest_data_p = dest.data();
        const ssize_t max_rc = dest.size();
        ssize_t done_rc = 0;
        auto has_lf = false;

        while (done_rc < max_rc) {
            if (const ssize_t temp_rc = recv(fd, dest.data() + done_rc, 1, 0); temp_rc > 0) {
                if (const char temp_c = dest_data_p[done_rc]; temp_c == cr_v) {
                    continue;
                } else if (temp_c == lf_v) {
                    dest_data_p[done_rc] = nul_v;
                    has_lf = true;
                    break;
                }

                ++done_rc;
            } else if (temp_rc == 0) {
                return {0};
            } else {
                return std::unexpected {"Invalid read with fd in io_funcs.cpp::socket_read_line(): temp_rc < 0"};
            }
        }

        if (!has_lf) {
            return std::unexpected {"Message line too large, exceeds buffer length."};
        }

        return {done_rc};
    }

    auto socket_write_n(int fd, ssize_t n, const ByteBuffer<>& src) noexcept -> IOResult<ssize_t> {
        const auto src_data_p = src.data();
        auto pending_wc = n;
        ssize_t done_wc = 0;

        while (pending_wc > 0) {
            if (const ssize_t temp_wc = send(fd, src_data_p + done_wc, pending_wc, 0); temp_wc > 0) {
                done_wc += temp_wc;
                pending_wc -= temp_wc;
            } else if (temp_wc == 0) {
                return {0};
            } else {
                return std::unexpected {"Bad write with fd in io_funcs.cpp::socket_write_n(): temp_wc < 0"};
            }
        }

        return {done_wc};
    }
}
