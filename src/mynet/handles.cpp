#include <unistd.h>

#include "mynet/handles.hpp"

namespace DerkHttpd::Net {
    Handles::Handles(pollfd pollable_fd)
    : m_pfds {} {
        m_pfds.emplace_back(pollable_fd);
    }

    Handles::~Handles() {
        if (m_pfds.empty()) {
            return;
        }

        for (const auto& pfd : m_pfds) {
            if (const auto fd_n = pfd.fd; fd_n > 0) {
                close(fd_n);
            }
        }

        m_pfds.clear();
    }
}
