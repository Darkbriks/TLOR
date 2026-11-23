#pragma once

#include <tlor/error.hpp>

#include <utility>

namespace tlor::test {

template <class F>
Status status_of(F&& f) {
    try {
        std::forward<F>(f)();
    } catch (const Error& e) {
        return e.status();
    }
    return Status::ok;
}

}
