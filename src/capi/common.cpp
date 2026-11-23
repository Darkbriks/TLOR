#include "capi/handles.hpp"

#include <tlor/version.hpp>

namespace tlor::capi {

namespace {

std::string& last_error_storage() {
    thread_local std::string message;
    return message;
}

}

void set_last_error(const std::string& message) { last_error_storage() = message; }

const char* last_error_message() { return last_error_storage().c_str(); }

}

extern "C" {

const char* tlor_version(void) { return tlor::library_version().data(); }

const char* tlor_status_name(tlor_status status) {
    if (status < TLOR_OK || status > TLOR_ERROR_INTERNAL) {
        return "unknown";
    }
    return tlor::to_string(static_cast<tlor::Status>(status)).data();
}

const char* tlor_last_error(void) { return tlor::capi::last_error_message(); }

void tlor_set_error(const char* message) { tlor::capi::set_last_error(message == nullptr ? "" : message); }

}
