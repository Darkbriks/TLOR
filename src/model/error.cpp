#include <tlor/error.hpp>

namespace tlor {

std::string_view to_string(Status status) {
    switch (status) {
        case Status::ok: return "ok";
        case Status::invalid_argument: return "invalid_argument";
        case Status::unknown_method: return "unknown_method";
        case Status::invalid_param: return "invalid_param";
        case Status::io_error: return "io_error";
        case Status::infeasible: return "infeasible";
        case Status::callback_error: return "callback_error";
        case Status::internal_error: return "internal_error";
    }
    return "internal_error";
}

Error::Error(Status status, const std::string& message) : std::runtime_error(message), status_(status) {}

Status Error::status() const noexcept { return status_; }

}
