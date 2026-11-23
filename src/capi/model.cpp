#include "capi/handles.hpp"

#include <vector>

using tlor::capi::emit;
using tlor::capi::guard;
using tlor::capi::need;
using tlor::capi::need_text;

namespace {

template <class T>
std::vector<T> copy_of(const T* data, std::size_t count, const char* what) {
    if (count > 0) {
        need(data, what);
    }
    return std::vector<T>(data, data + count);
}

std::size_t to_size(int32_t value) { return value > 0 ? static_cast<std::size_t>(value) : 0; }

}

extern "C" {

tlor_status tlor_instance_create(const tlor_instance_desc* desc, tlor_instance** out) {
    return guard([&] {
        need(out, "output pointer");
        *out = nullptr;
        const auto& d = need(desc, "desc");
        if (d.node_count < 1 || d.day_count < 1) {
            throw tlor::Error(tlor::Status::invalid_argument, "node_count and day_count must be positive");
        }
        const std::size_t n = to_size(d.node_count);
        tlor::InstanceData data{
            .node_count = d.node_count,
            .depot = d.depot,
            .day_count = d.day_count,
            .distance_m = copy_of(d.distance_m, n * n, "distance_m"),
            .duration_s = copy_of(d.duration_s, n * n, "duration_s"),
            .service_s = copy_of(d.service_s, n, "service_s"),
            .capacity_kg = copy_of(d.capacity_kg, n, "capacity_kg"),
            .active = copy_of(d.active, n, "active"),
            .demand_kg = copy_of(d.demand_kg, to_size(d.day_count) * n, "demand_kg"),
        };
        emit(out, tlor::Instance(std::move(data)));
    });
}

void tlor_instance_destroy(tlor_instance* instance) { delete instance; }

tlor_status tlor_instance_set_attribute(tlor_instance* instance, const char* name, const double* values) {
    return guard([&] {
        auto& inst = *need(instance, "instance").value;
        need_text(name, "name");
        inst.set_attribute(name, copy_of(values, to_size(inst.node_count()), "values"));
    });
}

int32_t tlor_instance_node_count(const tlor_instance* instance) { return instance ? instance->value->node_count() : 0; }

int32_t tlor_instance_depot(const tlor_instance* instance) { return instance ? instance->value->depot() : -1; }

int32_t tlor_instance_day_count(const tlor_instance* instance) { return instance ? instance->value->day_count() : 0; }

const double* tlor_instance_distance_m(const tlor_instance* instance) {
    return instance ? instance->value->distance_data().data() : nullptr;
}

const double* tlor_instance_duration_s(const tlor_instance* instance) {
    return instance ? instance->value->duration_data().data() : nullptr;
}

const double* tlor_instance_service_s(const tlor_instance* instance) {
    return instance ? instance->value->service_data().data() : nullptr;
}

const double* tlor_instance_capacity_kg(const tlor_instance* instance) {
    return instance ? instance->value->capacity_data().data() : nullptr;
}

const uint8_t* tlor_instance_active(const tlor_instance* instance) {
    return instance ? instance->value->active_data().data() : nullptr;
}

const double* tlor_instance_demand_kg(const tlor_instance* instance) {
    return instance ? instance->value->demand().data().data() : nullptr;
}

const double* tlor_instance_attribute(const tlor_instance* instance, const char* name) {
    if (instance == nullptr || name == nullptr) {
        return nullptr;
    }
    const std::vector<double>* values = instance->value->attribute(name);
    return values ? values->data() : nullptr;
}

tlor_status tlor_fleet_create(int32_t truck_count, const double* capacity_kg, const double* max_work_s,
                              int32_t day_count, const uint8_t* available, tlor_fleet** out) {
    return guard([&] {
        need(out, "output pointer");
        *out = nullptr;
        const std::size_t trucks = to_size(truck_count);
        const auto capacity = copy_of(capacity_kg, trucks, "capacity_kg");
        const auto work = copy_of(max_work_s, trucks, "max_work_s");
        std::vector<tlor::Truck> list;
        for (std::size_t t = 0; t < trucks; ++t) {
            list.push_back({capacity[t], work[t]});
        }
        std::vector<uint8_t> availability;
        if (available != nullptr) {
            availability.assign(available, available + to_size(day_count) * trucks);
        }
        *out = tlor::capi::fleet_handle(
                   std::make_shared<tlor::Fleet>(std::move(list), day_count, std::move(availability)))
                   .release();
    });
}

void tlor_fleet_destroy(tlor_fleet* fleet) { delete fleet; }

int32_t tlor_fleet_truck_count(const tlor_fleet* fleet) { return fleet ? fleet->value->truck_count() : 0; }

int32_t tlor_fleet_day_count(const tlor_fleet* fleet) { return fleet ? fleet->value->day_count() : 0; }

const double* tlor_fleet_capacity_kg(const tlor_fleet* fleet) { return fleet ? fleet->capacity_kg.data() : nullptr; }

const double* tlor_fleet_max_work_s(const tlor_fleet* fleet) { return fleet ? fleet->max_work_s.data() : nullptr; }

const uint8_t* tlor_fleet_available(const tlor_fleet* fleet) {
    return fleet ? fleet->value->available_data().data() : nullptr;
}

tlor_status tlor_state_create(int32_t node_count, const double* level_kg, const int32_t* last_collection,
                              tlor_state** out) {
    return guard([&] {
        need(out, "output pointer");
        *out = nullptr;
        if (node_count < 0) {
            throw tlor::Error(tlor::Status::invalid_argument, "node_count is negative");
        }
        tlor::State state = tlor::State::initial(node_count);
        const std::size_t n = to_size(node_count);
        if (level_kg != nullptr) {
            state.level_kg.assign(level_kg, level_kg + n);
        }
        if (last_collection != nullptr) {
            state.last_collection.assign(last_collection, last_collection + n);
        }
        emit(out, std::move(state));
    });
}

void tlor_state_destroy(tlor_state* state) { delete state; }

int32_t tlor_state_node_count(const tlor_state* state) {
    return state ? static_cast<int32_t>(state->value->level_kg.size()) : 0;
}

const double* tlor_state_level_kg(const tlor_state* state) { return state ? state->value->level_kg.data() : nullptr; }

const int32_t* tlor_state_last_collection(const tlor_state* state) {
    return state ? state->value->last_collection.data() : nullptr;
}

tlor_status tlor_params_create(tlor_params** out) {
    return guard([&] { emit(out, tlor::Params{}); });
}

void tlor_params_destroy(tlor_params* params) { delete params; }

tlor_status tlor_params_set_int(tlor_params* params, const char* name, int64_t value) {
    return guard([&] { need(params, "params").value->set(need_text(name, "name"), std::int64_t{value}); });
}

tlor_status tlor_params_set_double(tlor_params* params, const char* name, double value) {
    return guard([&] { need(params, "params").value->set(need_text(name, "name"), value); });
}

tlor_status tlor_params_set_bool(tlor_params* params, const char* name, int value) {
    return guard([&] { need(params, "params").value->set(need_text(name, "name"), value != 0); });
}

tlor_status tlor_params_set_string(tlor_params* params, const char* name, const char* value) {
    return guard([&] { need(params, "params").value->set(need_text(name, "name"), std::string(need_text(value, "value"))); });
}

size_t tlor_params_size(const tlor_params* params) { return params ? params->value->values().size() : 0; }

const char* tlor_params_key(const tlor_params* params, size_t index) {
    if (params == nullptr || index >= params->value->values().size()) {
        return nullptr;
    }
    auto it = params->value->values().begin();
    std::advance(it, static_cast<std::ptrdiff_t>(index));
    return it->first.c_str();
}

tlor_status tlor_params_type(const tlor_params* params, const char* name, tlor_param_type* out) {
    return guard([&] {
        need(out, "output pointer");
        const tlor::Value* value = need(params, "params").value->find(need_text(name, "name"));
        if (value == nullptr) {
            throw tlor::Error(tlor::Status::invalid_param, std::string("missing parameter '") + name + "'");
        }
        *out = static_cast<tlor_param_type>(value->index());
    });
}

tlor_status tlor_params_get_int(const tlor_params* params, const char* name, int64_t* out) {
    return guard([&] { need(out, "output pointer") = need(params, "params").value->get_int(need_text(name, "name")); });
}

tlor_status tlor_params_get_double(const tlor_params* params, const char* name, double* out) {
    return guard([&] { need(out, "output pointer") = need(params, "params").value->get_double(need_text(name, "name")); });
}

tlor_status tlor_params_get_bool(const tlor_params* params, const char* name, int* out) {
    return guard([&] { need(out, "output pointer") = need(params, "params").value->get_bool(need_text(name, "name")) ? 1 : 0; });
}

tlor_status tlor_params_get_string(const tlor_params* params, const char* name, const char** out) {
    return guard([&] {
        need(out, "output pointer") = need(params, "params").value->get_string(need_text(name, "name")).c_str();
    });
}

tlor_status tlor_plan_create(int32_t start_day, int32_t horizon, tlor_plan** out) {
    return guard([&] {
        need(out, "output pointer");
        *out = nullptr;
        emit(out, tlor::Plan(start_day, horizon));
    });
}

void tlor_plan_destroy(tlor_plan* plan) { delete plan; }

tlor_status tlor_plan_add(tlor_plan* plan, int32_t day, int32_t point) {
    return guard([&] { need(plan, "plan").value->add(day, point); });
}

tlor_status tlor_plan_remove(tlor_plan* plan, int32_t day, int32_t point) {
    return guard([&] {
        auto& p = *need(plan, "plan").value;
        if (!p.contains(day)) {
            throw tlor::Error(tlor::Status::invalid_argument, "day " + std::to_string(day) + " is outside the plan");
        }
        p.remove(day, point);
    });
}

int32_t tlor_plan_start(const tlor_plan* plan) { return plan ? plan->value->start() : 0; }

int32_t tlor_plan_horizon(const tlor_plan* plan) { return plan ? plan->value->horizon() : 0; }

tlor_status tlor_plan_points(const tlor_plan* plan, int32_t day, const int32_t** points, size_t* count) {
    return guard([&] {
        auto span = need(plan, "plan").value->points(day);
        need(points, "points") = span.data();
        need(count, "count") = span.size();
    });
}

tlor_status tlor_plan_assign(tlor_plan* dst, const tlor_plan* src) {
    return guard([&] { *need(dst, "dst").value = *need(src, "src").value; });
}

tlor_status tlor_day_routes_create(int32_t day, tlor_day_routes** out) {
    return guard([&] { emit(out, tlor::DayRoutes(day)); });
}

void tlor_day_routes_destroy(tlor_day_routes* routes) { delete routes; }

tlor_status tlor_day_routes_add(tlor_day_routes* routes, int32_t truck, const int32_t* nodes, size_t count) {
    return guard([&] {
        auto& r = *need(routes, "routes").value;
        r.add(tlor::Route{truck, copy_of(nodes, count, "nodes")});
    });
}

int32_t tlor_day_routes_day(const tlor_day_routes* routes) { return routes ? routes->value->day() : 0; }

size_t tlor_day_routes_count(const tlor_day_routes* routes) { return routes ? routes->value->size() : 0; }

tlor_status tlor_day_routes_get(const tlor_day_routes* routes, size_t index, int32_t* truck, const int32_t** nodes,
                                size_t* count) {
    return guard([&] {
        auto all = need(routes, "routes").value->routes();
        if (index >= all.size()) {
            throw tlor::Error(tlor::Status::invalid_argument, "route index " + std::to_string(index) + " is out of range");
        }
        const tlor::Route& route = all[index];
        need(truck, "truck") = route.truck;
        need(nodes, "nodes") = route.nodes.data();
        need(count, "count") = route.nodes.size();
    });
}

tlor_status tlor_day_routes_assign(tlor_day_routes* dst, const tlor_day_routes* src) {
    return guard([&] { *need(dst, "dst").value = *need(src, "src").value; });
}

tlor_status tlor_solution_create(const tlor_plan* plan, tlor_solution** out) {
    return guard([&] { emit(out, tlor::Solution(*need(plan, "plan").value)); });
}

void tlor_solution_destroy(tlor_solution* solution) { delete solution; }

const tlor_plan* tlor_solution_plan(const tlor_solution* solution) {
    if (solution == nullptr) {
        return nullptr;
    }
    if (!solution->plan_view) {
        solution->plan_view = std::make_unique<tlor_plan>(
            tlor_plan{std::shared_ptr<tlor::Plan>(solution->value, &solution->value->plan())});
    }
    return solution->plan_view.get();
}

tlor_status tlor_solution_set_routes(tlor_solution* solution, const tlor_day_routes* routes) {
    return guard([&] { need(solution, "solution").value->set_routes(*need(routes, "routes").value); });
}

const tlor_day_routes* tlor_solution_routes(const tlor_solution* solution, int32_t day) {
    if (solution == nullptr) {
        return nullptr;
    }
    const tlor::DayRoutes* routes = solution->value->routes(day);
    if (routes == nullptr) {
        return nullptr;
    }
    auto& slot = solution->route_views[day];
    if (!slot) {
        slot = std::make_unique<tlor_day_routes>();
    }
    slot->value = std::shared_ptr<tlor::DayRoutes>(solution->value, const_cast<tlor::DayRoutes*>(routes));
    return slot.get();
}

tlor_status tlor_solution_assign(tlor_solution* dst, const tlor_solution* src) {
    return guard([&] { *need(dst, "dst").value = *need(src, "src").value; });
}

}
