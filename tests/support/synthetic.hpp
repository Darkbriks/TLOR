#pragma once

#include <tlor/error.hpp>
#include <tlor/fleet.hpp>
#include <tlor/instance.hpp>
#include <tlor/registry.hpp>

#include <cstdint>
#include <string>
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

class ScopedMethod {
public:
    ScopedMethod(Step step, std::string name) : step_(step), name_(std::move(name)) {}
    ~ScopedMethod() {
        try {
            Registry::global().remove(step_, name_);
        } catch (const Error&) {
        }
    }
    ScopedMethod(const ScopedMethod&) = delete;
    ScopedMethod& operator=(const ScopedMethod&) = delete;

private:
    Step step_;
    std::string name_;
};

}
