#ifndef DERK_HTTPD_MAKE_SRVSOCK_HPP
#define DERK_HTTPD_MAKE_SRVSOCK_HPP

#include <iostream>
#include <print>
#include <unistd.h>
#include <poll.h>
#include <netdb.h>

#include <string_view>
#include <optional>

#include "mynet/enums.hpp"

namespace DerkHttpd::Net {
    class CreateServerSocket {
    private:
        addrinfo* m_head_p;
        addrinfo* m_next_p;
        int m_backlog_n;
        short m_event_mask;

    public:
        template <std::same_as<PollEvent> FirstEv, std::same_as<PollEvent> ... Evs>
        CreateServerSocket(std::string_view port_cstr, int backlog_n, FirstEv first_ev, Evs ... rest_evs) noexcept
        : m_head_p {}, m_next_p {}, m_backlog_n {backlog_n}, m_event_mask ((static_cast<short>(first_ev) | ... | static_cast<short>(rest_evs))) {
            addrinfo host_hints {};
            host_hints.ai_family = AF_INET;
            host_hints.ai_socktype = SOCK_STREAM;
            host_hints.ai_flags = AI_PASSIVE;

            if (const auto setup_status = getaddrinfo(nullptr, port_cstr.data(), &host_hints, &m_head_p); setup_status != 0) {
                std::println(std::cerr, "SETUP ERROR: {}", gai_strerror(setup_status));
            } else {
                m_next_p = m_head_p;
            }
        }

        ~CreateServerSocket() noexcept;

        CreateServerSocket(const CreateServerSocket&) = delete;
        CreateServerSocket& operator=(const CreateServerSocket&) = delete;
        CreateServerSocket(CreateServerSocket&&) = delete;
        CreateServerSocket& operator=(CreateServerSocket&&) = delete;

        [[nodiscard]] auto operator()() noexcept -> std::optional<pollfd>;
    };
}

#endif
