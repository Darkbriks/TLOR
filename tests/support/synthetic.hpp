#pragma once

#include <tlor/error.hpp>
#include <tlor/fleet.hpp>
#include <tlor/instance.hpp>

#include <cstdint>
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

Instance small_instance();

Fleet small_fleet(double capacity_kg = 150, double max_work_s = 3600, std::int32_t trucks = 2);

}
