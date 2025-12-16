#include <chrono>
#include <format>
#include <iomanip>
#include <sstream>

#include "myapp/response_helpers.hpp"

namespace DerkHttpd::App {
    auto get_date_string() -> std::string {
        auto now_time = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now());

        return std::format("{0:%a}, {0:%e} {0:%b} {0:%Y} {0:%H}:{0:%M}:{0:%S} UTC", now_time);
    }

    auto parse_date_string(const std::string& date) -> std::chrono::seconds {
        // 1. Parse formatted GMT / UTC date from the client.
        std::istringstream str_reader {date};
        std::tm date_tm = {};

        str_reader >> std::get_time(&date_tm, "%a, %e %b %Y %H:%M:%S GMT");

        // 2. Convert the raw std::tm from ctime to the since-epoch duraction in microseconds.
        auto date_epoch_us = std::chrono::system_clock::from_time_t(std::mktime(&date_tm)).time_since_epoch();

        // 3. Convert to std::chrono::seconds, since the original format only has precision in seconds.
        return std::chrono::duration_cast<std::chrono::seconds>(date_epoch_us);
    }

    auto get_epoch_seconds_now() -> std::chrono::seconds {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        );
    }
}