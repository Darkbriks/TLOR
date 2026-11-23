#include <tlor/io.hpp>

#include <tlor/error.hpp>

#include <charconv>
#include <cstdio>

namespace tlor {

namespace {

bool read_digits(std::string_view text, int& value) {
    for (char c : text) {
        if (c < '0' || c > '9') {
            return false;
        }
    }
    return std::from_chars(text.data(), text.data() + text.size(), value).ec == std::errc{};
}

}

std::chrono::sys_days parse_date(std::string_view text) {
    int year = 0;
    int month = 0;
    int day = 0;
    if (text.size() != 10 || text[4] != '-' || text[7] != '-' || !read_digits(text.substr(0, 4), year) ||
        !read_digits(text.substr(5, 2), month) || !read_digits(text.substr(8, 2), day)) {
        throw Error(Status::invalid_argument, "invalid date '" + std::string(text) + "', expected YYYY-MM-DD");
    }
    const std::chrono::year_month_day date{std::chrono::year{year}, std::chrono::month{static_cast<unsigned>(month)},
                                           std::chrono::day{static_cast<unsigned>(day)}};
    if (!date.ok()) {
        throw Error(Status::invalid_argument, "invalid date '" + std::string(text) + "'");
    }
    return std::chrono::sys_days{date};
}

std::string format_date(std::chrono::sys_days date) {
    const std::chrono::year_month_day ymd{date};
    char text[32];
    const int size = std::snprintf(text, sizeof text, "%04d-%02u-%02u", static_cast<int>(ymd.year()),
                                   static_cast<unsigned>(ymd.month()), static_cast<unsigned>(ymd.day()));
    return {text, static_cast<std::size_t>(size)};
}

}
