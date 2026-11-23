/**
 * @file error.hpp
 * @brief Status codes and the exception thrown by the library.
 */
#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

namespace tlor {

/** @brief Outcome of a library call, shared with the C interface. */
enum class Status : int {
    ok = 0,
    invalid_argument = 1,
    unknown_method = 2,
    invalid_param = 3,
    io_error = 4,
    infeasible = 5,
    callback_error = 6,
    internal_error = 7,
};

/** @brief Name of a status, such as "invalid_argument". */
std::string_view to_string(Status status);

/** @brief Exception carrying a status and a message. */
class Error : public std::runtime_error {
public:
    /** @brief Builds an error with its status and message. */
    Error(Status status, const std::string& message);

    /** @brief Status of the error. */
    [[nodiscard]] Status status() const noexcept;

private:
    Status status_;
};

}
