#ifndef DERK_HTTPD_NET_ENUMS_HPP
#define DERK_HTTPD_NET_ENUMS_HPP

#include <poll.h>

namespace DerkHttpd::Net {   
    enum class PollEvent : short {
        idle = 0x0,
        received = POLLIN,
        hangup = POLLHUP,
    };
}

#endif