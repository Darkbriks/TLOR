#include "capi/handles.hpp"

#include <tlor/registry.hpp>

#include <string>

using tlor::capi::borrow;
using tlor::capi::guard;
using tlor::capi::need;
using tlor::capi::need_text;

namespace {

tlor_report* borrowed_report(tlor::Report* report, tlor_report& slot) {
    if (report == nullptr) {
        return nullptr;
    }
    slot.value = borrow(*report);
    return &slot;
}

struct Common {
    tlor_instance instance;
    std::unique_ptr<tlor_fleet> fleet;
    tlor_params params;
    tlor_params weights;
    tlor_report report_slot;
    tlor_report* report = nullptr;

    Common(const tlor::Instance& i, const tlor::Fleet& f, const tlor::Params& p, const tlor::Params& w, tlor::Report* r)
        : instance{borrow(i)}, fleet(tlor::capi::fleet_handle(borrow(f))), params{borrow(p)}, weights{borrow(w)} {
        report = borrowed_report(r, report_slot);
    }
};

[[noreturn]] void callback_failed(tlor_status status) {
    std::string message = tlor::capi::last_error_message();
    if (message.empty()) {
        message = std::string("callback failed with status ") + tlor_status_name(status);
    }
    throw tlor::Error(tlor::Status::callback_error, message);
}

tlor::Step step_of(tlor_step step) {
    if (step < TLOR_STEP_SCHEDULE || step > TLOR_STEP_IMPROVE) {
        throw tlor::Error(tlor::Status::invalid_argument, "unknown step " + std::to_string(static_cast<int>(step)));
    }
    return static_cast<tlor::Step>(step);
}

tlor::MethodInfo info_of(const char* name, const char* description, const tlor_param_spec* specs, size_t count) {
    tlor::MethodInfo info{need_text(name, "name"), description == nullptr ? "" : description, {}, false};
    if (count > 0) {
        need(specs, "specs");
    }
    for (size_t i = 0; i < count; ++i) {
        const tlor_param_spec& s = specs[i];
        tlor::ParamSpec spec;
        spec.name = need_text(s.name, "parameter name");
        switch (s.type) {
            case TLOR_PARAM_INT: spec.type = tlor::ParamType::integer; spec.default_value = std::int64_t{s.default_int}; break;
            case TLOR_PARAM_DOUBLE: spec.type = tlor::ParamType::real; spec.default_value = s.default_double; break;
            case TLOR_PARAM_BOOL: spec.type = tlor::ParamType::boolean; spec.default_value = s.default_bool != 0; break;
            case TLOR_PARAM_STRING:
                spec.type = tlor::ParamType::string;
                spec.default_value = std::string(s.default_string == nullptr ? "" : s.default_string);
                break;
            default: throw tlor::Error(tlor::Status::invalid_argument, "parameter '" + spec.name + "' has an unknown type");
        }
        if (s.has_min != 0) {
            spec.min = s.min;
        }
        if (s.has_max != 0) {
            spec.max = s.max;
        }
        spec.description = s.description == nullptr ? "" : s.description;
        info.params.push_back(std::move(spec));
    }
    return info;
}

}

struct tlor_schedule_request {
    const tlor::ScheduleRequest& request;
    Common common;
    tlor_state state;

    explicit tlor_schedule_request(const tlor::ScheduleRequest& r)
        : request(r), common(r.instance, r.fleet, r.params, r.weights, r.report), state{borrow(r.start_state)} {}
};

struct tlor_route_request {
    const tlor::RouteRequest& request;
    Common common;

    explicit tlor_route_request(const tlor::RouteRequest& r)
        : request(r), common(r.instance, r.fleet, r.params, r.weights, r.report) {}
};

struct tlor_improve_request {
    const tlor::ImproveRequest& request;
    Common common;
    tlor_state state;
    tlor_solution solution;
    tlor_params route_params;
    std::string route_method;

    explicit tlor_improve_request(const tlor::ImproveRequest& r)
        : request(r), common(r.instance, r.fleet, r.params, r.weights, r.report), state{borrow(r.start_state)},
          solution{borrow(r.solution), nullptr, {}}, route_params{borrow(r.route_params)},
          route_method(r.route_method) {}
};

extern "C" {

size_t tlor_method_count(tlor_step step) {
    try {
        return tlor::Registry::global().methods(step_of(step)).size();
    } catch (...) {
        return 0;
    }
}

const char* tlor_method_name(tlor_step step, size_t index) {
    try {
        auto methods = tlor::Registry::global().methods(step_of(step));
        return index < methods.size() ? methods[index]->name.c_str() : nullptr;
    } catch (...) {
        return nullptr;
    }
}

const char* tlor_method_description(tlor_step step, const char* name) {
    try {
        return tlor::Registry::global().info(step_of(step), need_text(name, "name")).description.c_str();
    } catch (...) {
        return nullptr;
    }
}

tlor_status tlor_method_param_count(tlor_step step, const char* method, size_t* out) {
    return guard([&] {
        need(out, "output pointer") = tlor::Registry::global().info(step_of(step), need_text(method, "method")).params.size();
    });
}

tlor_status tlor_method_param(tlor_step step, const char* method, size_t index, tlor_param_spec* out) {
    return guard([&] {
        const auto& params = tlor::Registry::global().info(step_of(step), need_text(method, "method")).params;
        if (index >= params.size()) {
            throw tlor::Error(tlor::Status::invalid_argument, "parameter index " + std::to_string(index) + " is out of range");
        }
        const tlor::ParamSpec& spec = params[index];
        tlor_param_spec result{};
        result.name = spec.name.c_str();
        result.type = static_cast<tlor_param_type>(spec.type);
        if (const auto* i = std::get_if<std::int64_t>(&spec.default_value)) {
            result.default_int = *i;
        } else if (const auto* d = std::get_if<double>(&spec.default_value)) {
            result.default_double = *d;
        } else if (const auto* b = std::get_if<bool>(&spec.default_value)) {
            result.default_bool = *b ? 1 : 0;
        } else if (const auto* s = std::get_if<std::string>(&spec.default_value)) {
            result.default_string = s->c_str();
        }
        result.has_min = spec.min ? 1 : 0;
        result.min = spec.min.value_or(0.0);
        result.has_max = spec.max ? 1 : 0;
        result.max = spec.max.value_or(0.0);
        result.description = spec.description.c_str();
        need(out, "output pointer") = result;
    });
}

const tlor_instance* tlor_schedule_request_instance(const tlor_schedule_request* r) { return r ? &r->common.instance : nullptr; }
const tlor_fleet* tlor_schedule_request_fleet(const tlor_schedule_request* r) { return r ? r->common.fleet.get() : nullptr; }
const tlor_state* tlor_schedule_request_state(const tlor_schedule_request* r) { return r ? &r->state : nullptr; }
int32_t tlor_schedule_request_start_day(const tlor_schedule_request* r) { return r ? r->request.start_day : 0; }
int32_t tlor_schedule_request_horizon(const tlor_schedule_request* r) { return r ? r->request.horizon : 0; }
const tlor_params* tlor_schedule_request_params(const tlor_schedule_request* r) { return r ? &r->common.params : nullptr; }
const tlor_params* tlor_schedule_request_weights(const tlor_schedule_request* r) { return r ? &r->common.weights : nullptr; }
tlor_report* tlor_schedule_request_report(const tlor_schedule_request* r) { return r ? r->common.report : nullptr; }

const tlor_instance* tlor_route_request_instance(const tlor_route_request* r) { return r ? &r->common.instance : nullptr; }
const tlor_fleet* tlor_route_request_fleet(const tlor_route_request* r) { return r ? r->common.fleet.get() : nullptr; }
int32_t tlor_route_request_day(const tlor_route_request* r) { return r ? r->request.day : 0; }
size_t tlor_route_request_points(const tlor_route_request* r, const int32_t** points) {
    if (r == nullptr) {
        return 0;
    }
    if (points != nullptr) {
        *points = r->request.points.data();
    }
    return r->request.points.size();
}
const double* tlor_route_request_loads_kg(const tlor_route_request* r) { return r ? r->request.loads_kg.data() : nullptr; }
const tlor_params* tlor_route_request_params(const tlor_route_request* r) { return r ? &r->common.params : nullptr; }
const tlor_params* tlor_route_request_weights(const tlor_route_request* r) { return r ? &r->common.weights : nullptr; }
tlor_report* tlor_route_request_report(const tlor_route_request* r) { return r ? r->common.report : nullptr; }

const tlor_instance* tlor_improve_request_instance(const tlor_improve_request* r) { return r ? &r->common.instance : nullptr; }
const tlor_fleet* tlor_improve_request_fleet(const tlor_improve_request* r) { return r ? r->common.fleet.get() : nullptr; }
const tlor_state* tlor_improve_request_state(const tlor_improve_request* r) { return r ? &r->state : nullptr; }
const tlor_solution* tlor_improve_request_solution(const tlor_improve_request* r) { return r ? &r->solution : nullptr; }
const char* tlor_improve_request_route_method(const tlor_improve_request* r) { return r ? r->route_method.c_str() : nullptr; }
const tlor_params* tlor_improve_request_route_params(const tlor_improve_request* r) { return r ? &r->route_params : nullptr; }
const tlor_params* tlor_improve_request_params(const tlor_improve_request* r) { return r ? &r->common.params : nullptr; }
const tlor_params* tlor_improve_request_weights(const tlor_improve_request* r) { return r ? &r->common.weights : nullptr; }
tlor_report* tlor_improve_request_report(const tlor_improve_request* r) { return r ? r->common.report : nullptr; }

tlor_status tlor_register_schedule_method(const char* name, const char* description, const tlor_param_spec* specs,
                                          size_t spec_count, tlor_schedule_fn fn, void* user_data) {
    return guard([&] {
        need(fn, "callback");
        tlor::Registry::global().add_schedule(info_of(name, description, specs, spec_count),
                                              [fn, user_data](const tlor::ScheduleRequest& r) {
                                                  tlor_schedule_request request(r);
                                                  tlor_plan out{std::make_shared<tlor::Plan>(r.start_day, r.horizon)};
                                                  tlor::capi::set_last_error("");
                                                  const tlor_status status = fn(&request, &out, user_data);
                                                  if (status != TLOR_OK) {
                                                      callback_failed(status);
                                                  }
                                                  return *out.value;
                                              });
    });
}

tlor_status tlor_register_route_method(const char* name, const char* description, const tlor_param_spec* specs,
                                       size_t spec_count, tlor_route_fn fn, void* user_data) {
    return guard([&] {
        need(fn, "callback");
        tlor::Registry::global().add_route(info_of(name, description, specs, spec_count),
                                           [fn, user_data](const tlor::RouteRequest& r) {
                                               tlor_route_request request(r);
                                               tlor_day_routes out{std::make_shared<tlor::DayRoutes>(r.day)};
                                               tlor::capi::set_last_error("");
                                               const tlor_status status = fn(&request, &out, user_data);
                                               if (status != TLOR_OK) {
                                                   callback_failed(status);
                                               }
                                               return *out.value;
                                           });
    });
}

tlor_status tlor_register_improve_method(const char* name, const char* description, const tlor_param_spec* specs,
                                         size_t spec_count, tlor_improve_fn fn, void* user_data) {
    return guard([&] {
        need(fn, "callback");
        tlor::Registry::global().add_improve(info_of(name, description, specs, spec_count),
                                             [fn, user_data](const tlor::ImproveRequest& r) {
                                                 tlor_improve_request request(r);
                                                 tlor_solution out{std::make_shared<tlor::Solution>(r.solution), nullptr, {}};
                                                 tlor::capi::set_last_error("");
                                                 const tlor_status status = fn(&request, &out, user_data);
                                                 if (status != TLOR_OK) {
                                                     callback_failed(status);
                                                 }
                                                 return *out.value;
                                             });
    });
}

tlor_status tlor_unregister_method(tlor_step step, const char* name) {
    return guard([&] { tlor::Registry::global().remove(step_of(step), need_text(name, "name")); });
}

}
