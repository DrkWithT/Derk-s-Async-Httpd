#include <unistd.h>
#include <netdb.h>

#include "mynet/make_srvsock.hpp"

namespace DerkHttpd::Net {
    CreateServerSocket::~CreateServerSocket() noexcept {
        if (m_head_p != nullptr) {
            freeaddrinfo(m_head_p);
            m_head_p = nullptr;
        }
    }

    auto CreateServerSocket::operator()() noexcept -> std::optional<pollfd> {
        if (!m_next_p) {
            return {};
        }

        auto temp_fd = socket(m_next_p->ai_family, m_next_p->ai_socktype, m_next_p->ai_protocol);

        if (temp_fd == -1) {
            return {{.fd = -1}};
        }

        if (bind(temp_fd, m_next_p->ai_addr, m_next_p->ai_addrlen) < 0) {
            close(temp_fd);

            return {{.fd = -1}};
        }

        if (listen(temp_fd, m_backlog_n) == -1) {
            close(temp_fd);

            return {{.fd = -1}};
        }

        return pollfd {
            .fd = temp_fd,
            .events = m_event_mask,
            .revents = 0,
        };
    }
}
