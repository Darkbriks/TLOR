#pragma once

#include <tlor/error.hpp>
#include <tlor/fleet.hpp>
#include <tlor/instance.hpp>
#include <tlor/params.hpp>
#include <tlor/plan.hpp>
#include <tlor/routes.hpp>
#include <tlor/solution.hpp>
#include <tlor/state.hpp>
#include <tlor/tlor.h>

#include <map>
#include <memory>
#include <new>
#include <string>
#include <vector>

struct tlor_instance {
    std::shared_ptr<tlor::Instance> value;
};

struct tlor_fleet {
    std::shared_ptr<tlor::Fleet> value;
    std::vector<double> capacity_kg;
    std::vector<double> max_work_s;
};

struct tlor_state {
    std::shared_ptr<tlor::State> value;
};

struct tlor_params {
    std::shared_ptr<tlor::Params> value;
};

struct tlor_plan {
    std::shared_ptr<tlor::Plan> value;
};

struct tlor_day_routes {
    std::shared_ptr<tlor::DayRoutes> value;
};

struct tlor_solution {
    std::shared_ptr<tlor::Solution> value;
    mutable std::unique_ptr<tlor_plan> plan_view;
    mutable std::map<int32_t, std::unique_ptr<tlor_day_routes>> route_views;
};

namespace tlor::capi {

void set_last_error(const std::string& message);

const char* last_error_message();

template <class F>
tlor_status guard(F&& f) noexcept {
    set_last_error("");
    try {
        f();
        return TLOR_OK;
    } catch (const Error& e) {
        set_last_error(e.what());
        return static_cast<tlor_status>(e.status());
    } catch (const std::bad_alloc&) {
        set_last_error("out of memory");
        return TLOR_ERROR_INTERNAL;
    } catch (const std::exception& e) {
        set_last_error(e.what());
        return TLOR_ERROR_INTERNAL;
    } catch (...) {
        set_last_error("unknown error");
        return TLOR_ERROR_INTERNAL;
    }
}

template <class T>
T& need(T* pointer, const char* what) {
    if (pointer == nullptr) {
        throw Error(Status::invalid_argument, std::string(what) + " is null");
    }
    return *pointer;
}

inline const char* need_text(const char* text, const char* what) {
    if (text == nullptr) {
        throw Error(Status::invalid_argument, std::string(what) + " is null");
    }
    return text;
}

template <class Handle, class T>
void emit(Handle** out, T&& value) {
    need(out, "output pointer");
    *out = new Handle{std::make_shared<std::decay_t<T>>(std::forward<T>(value))};
}

template <class T>
std::shared_ptr<T> borrow(const T& object) {
    return std::shared_ptr<T>(const_cast<T*>(&object), [](T*) {});
}

inline std::unique_ptr<tlor_fleet> fleet_handle(std::shared_ptr<Fleet> fleet) {
    auto handle = std::make_unique<tlor_fleet>();
    for (TruckId t = 0; t < fleet->truck_count(); ++t) {
        handle->capacity_kg.push_back(fleet->truck(t).capacity_kg);
        handle->max_work_s.push_back(fleet->truck(t).max_work_s);
    }
    handle->value = std::move(fleet);
    return handle;
}

inline const Params& params_or_empty(const tlor_params* params) {
    static const Params empty;
    return params == nullptr ? empty : *params->value;
}

}
