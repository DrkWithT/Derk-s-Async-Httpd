#ifndef DERK_HTTPD_MYNET_HANDLES_HPP
#define DERK_HTTPD_MYNET_HANDLES_HPP

#include <unistd.h>
#include <sys/socket.h>
#include <poll.h>

#include <type_traits>
#include <expected>
#include <algorithm>
#include <string>
#include <set>
#include <vector>
#include <future>

#include "mynet/enums.hpp"

namespace DerkHttpd::Net {
    struct IOTaskResult {
        int pollfd_idx;
        bool ok;
    };

    class Handles {
    private:
        static constexpr auto fallback_timeout = 15;
        static constexpr auto poll_error_n = -1;

        std::vector<pollfd> m_pfds; // pollable BSD socket handles
    public:
        /// NOTE: `pollable_fd` will be appended 1st to the fd pool.
        explicit Handles(pollfd pollable_fd);
        ~Handles();

        Handles(const Handles&) = delete;
        Handles& operator=(const Handles&) = delete;
        Handles(Handles&&) = delete;
        Handles& operator=(Handles&&) = delete;

        template <typename Fn, std::same_as<PollEvent> FirstEv, std::same_as<PollEvent> ... Evs> requires (std::is_invocable_r_v<IOTaskResult, Fn, int, int>)
        [[nodiscard]] auto dispatch_active_fds(Fn& callable, FirstEv first_event_tag, Evs ... event_tags) noexcept -> std::expected<int, std::string> {
            const auto poll_n = poll(m_pfds.data(), m_pfds.size(), fallback_timeout);

            if (poll_n == poll_error_n) {
                return std::unexpected {"Handles::dispatch_active_fds: failed to poll any fd."};
            }

            std::vector<std::future<IOTaskResult>> task_statuses;
            auto pfd_n = static_cast<int>(m_pfds.size());

            for (int fd_index = 0; fd_index < pfd_n; ++fd_index) {
                const auto& pfd = m_pfds[fd_index];

                if ( (pfd.revents & (static_cast<short>(first_event_tag) | ... | static_cast<short>(event_tags)) ) == 0) {
                    continue;
                }

                // 1. Handle listener event
                if (fd_index == 0) {
                    if (auto incoming_fd = accept(pfd.fd, nullptr, nullptr); incoming_fd != -1) {
                        m_pfds.emplace_back(pollfd {
                            .fd = incoming_fd,
                            .events = POLLIN,
                            .revents = 0,
                        });
                        ++pfd_n;
                    }
                } else {
                    // 2. Handle client socket event
                    task_statuses.emplace_back(std::async(std::launch::async, callable, fd_index, pfd.fd));
                }
            }

            std::set<int> evicting_fds;

            for (auto& io_promise : task_statuses) {
                if (auto [io_task_fd_idx, io_task_status] = io_promise.get(); io_task_status) {
                    ; // The task here finished well, so just keep awaiting for any error or until the end.
                } else {
                    evicting_fds.emplace(m_pfds.at(io_task_fd_idx).fd);
                }
            }

            auto evict_count = 0;

            std::ranges::partition(m_pfds, [&, this](const pollfd& item) -> bool {
                if (evicting_fds.contains(item.fd)) {
                    ++evict_count;
                    return false;
                }

                return true;
            });

            for (; evict_count > 0; --evict_count) {
                close(m_pfds.back().fd);
                m_pfds.pop_back();
            }

            return {poll_n};
        }
    };
}

#endif
