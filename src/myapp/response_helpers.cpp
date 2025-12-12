#include <chrono>
#include <format>

#include "myapp/response_helpers.hpp"

namespace DerkHttpd::App {
    auto get_date_string() -> std::string {
        auto now_time = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now());

        return std::format("{0:%a}, {0:%e} {0:%b} {0:%Y} {0:%H}:{0:%M}:{0:%S} UTC", now_time);
    }
}