#include "capi/handles.hpp"

#include <tlor/simulate.hpp>
#include <tlor/steps.hpp>

#include <algorithm>
#include <optional>
#include <span>

using tlor::capi::demand_of;
using tlor::capi::guard;
using tlor::capi::need;
using tlor::capi::need_text;
using tlor::capi::params_or_empty;
using tlor::capi::state_or_initial;

namespace {

tlor::Report* report_of(tlor_report* report) { return report == nullptr ? nullptr : report->value.get(); }

template <class Handle, class T>
void give(Handle** out, T&& value) {
    *out = new Handle{std::make_shared<std::decay_t<T>>(std::forward<T>(value))};
}

}

extern "C" {

tlor_status tlor_schedule(const tlor_instance* instance, const tlor_fleet* fleet, const tlor_state* state,
                          int32_t start_day, int32_t horizon, const char* method, const tlor_params* params,
                          const tlor_params* weights, tlor_report* report, tlor_plan** out) {
    return guard([&] {
        need(out, "output pointer") = nullptr;
        const auto& inst = *need(instance, "instance").value;
        give(out, tlor::schedule(inst, *need(fleet, "fleet").value, state_or_initial(state, inst), start_day, horizon,
                                 need_text(method, "method"), params_or_empty(params), params_or_empty(weights),
                                 report_of(report)));
    });
}

tlor_status tlor_route(const tlor_instance* instance, const tlor_fleet* fleet, int32_t day, const int32_t* points,
                       const double* loads_kg, size_t count, const char* method, const tlor_params* params,
                       const tlor_params* weights, tlor_report* report, tlor_day_routes** out) {
    return guard([&] {
        need(out, "output pointer") = nullptr;
        if (count > 0) {
            need(points, "points");
            need(loads_kg, "loads_kg");
        }
        give(out, tlor::route(*need(instance, "instance").value, *need(fleet, "fleet").value, day,
                              std::span<const tlor::Node>(points, count), std::span<const double>(loads_kg, count),
                              need_text(method, "method"), params_or_empty(params), params_or_empty(weights),
                              report_of(report)));
    });
}

tlor_status tlor_improve(const tlor_instance* instance, const tlor_fleet* fleet, const tlor_state* state,
                         const tlor_solution* solution, const char* route_method, const tlor_params* route_params,
                         const char* method, const tlor_params* params, const tlor_params* weights,
                         tlor_report* report, tlor_solution** out) {
    return guard([&] {
        need(out, "output pointer") = nullptr;
        const auto& inst = *need(instance, "instance").value;
        const auto& input = *need(solution, "solution").value;
        tlor::Solution result = tlor::improve(inst, *need(fleet, "fleet").value, state_or_initial(state, inst), input,
                                              need_text(route_method, "route_method"), params_or_empty(route_params),
                                              need_text(method, "method"), params_or_empty(params),
                                              params_or_empty(weights), report_of(report));
        *out = new tlor_solution{std::make_shared<tlor::Solution>(std::move(result)), nullptr, {}};
    });
}

tlor_status tlor_simulate(const tlor_instance* instance, const tlor_state* state, int32_t start_day, int32_t days,
                          const tlor_plan* plan, const double* demand_kg, tlor_state** end_state, double* level_kg) {
    return guard([&] {
        if (end_state != nullptr) {
            *end_state = nullptr;
        }
        const auto& inst = *need(instance, "instance").value;
        const auto demand = demand_of(inst, demand_kg);
        tlor::SimulationResult result = tlor::simulate(inst, state_or_initial(state, inst), start_day, days,
                                                       *need(plan, "plan").value, demand ? *demand : inst.demand());
        if (level_kg != nullptr) {
            std::ranges::copy(result.level_kg, level_kg);
        }
        if (end_state != nullptr) {
            give(end_state, std::move(result.end));
        }
    });
}

tlor_status tlor_evaluate(const tlor_instance* instance, const tlor_fleet* fleet, const tlor_state* state,
                          const tlor_solution* solution, const double* demand_kg, const tlor_params* evaluation_params,
                          tlor_evaluation** out) {
    return guard([&] {
        need(out, "output pointer") = nullptr;
        const auto& inst = *need(instance, "instance").value;
        const auto demand = demand_of(inst, demand_kg);
        auto evaluation = std::make_shared<tlor::Evaluation>(
            tlor::evaluate(inst, *need(fleet, "fleet").value, state_or_initial(state, inst),
                           *need(solution, "solution").value, demand ? *demand : inst.demand(),
                           params_or_empty(evaluation_params)));
        *out = new tlor_evaluation(tlor::capi::evaluation_handle(std::move(evaluation)));
    });
}

void tlor_evaluation_destroy(tlor_evaluation* evaluation) { delete evaluation; }

const tlor_indicators* tlor_evaluation_total(const tlor_evaluation* evaluation) {
    return evaluation == nullptr ? nullptr : &evaluation->total;
}

int32_t tlor_evaluation_start_day(const tlor_evaluation* evaluation) {
    return evaluation == nullptr ? 0 : evaluation->value->start_day;
}

int32_t tlor_evaluation_day_count(const tlor_evaluation* evaluation) {
    return evaluation == nullptr ? 0 : static_cast<int32_t>(evaluation->days.size());
}

const tlor_indicators* tlor_evaluation_day(const tlor_evaluation* evaluation, int32_t offset) {
    if (evaluation == nullptr || offset < 0 || static_cast<std::size_t>(offset) >= evaluation->days.size()) {
        return nullptr;
    }
    return &evaluation->days[static_cast<std::size_t>(offset)];
}

size_t tlor_indicator_count(void) { return tlor::indicator_names.size(); }

const char* tlor_indicator_name(size_t index) {
    return index < tlor::indicator_names.size() ? tlor::indicator_names[index].data() : nullptr;
}

tlor_status tlor_indicators_get(const tlor_indicators* indicators, const char* name, double* out) {
    return guard([&] { need(out, "output pointer") = need(indicators, "indicators").value->at(need_text(name, "name")); });
}

tlor_status tlor_objective(const tlor_indicators* indicators, const tlor_params* weights, double* out) {
    return guard([&] {
        need(out, "output pointer") = tlor::objective(*need(indicators, "indicators").value, params_or_empty(weights));
    });
}

tlor_status tlor_check(const tlor_instance* instance, const tlor_fleet* fleet, const tlor_state* state,
                       const tlor_solution* solution, const double* demand_kg, tlor_violations** out) {
    return guard([&] {
        need(out, "output pointer") = nullptr;
        const auto& inst = *need(instance, "instance").value;
        const auto demand = demand_of(inst, demand_kg);
        give(out, tlor::check(inst, *need(fleet, "fleet").value, state_or_initial(state, inst),
                              *need(solution, "solution").value, demand ? *demand : inst.demand()));
    });
}

void tlor_violations_destroy(tlor_violations* violations) { delete violations; }

size_t tlor_violations_count(const tlor_violations* violations) {
    return violations == nullptr ? 0 : violations->value->size();
}

tlor_status tlor_violations_get(const tlor_violations* violations, size_t index, tlor_violation* out) {
    return guard([&] {
        const auto& list = *need(violations, "violations").value;
        if (index >= list.size()) {
            throw tlor::Error(tlor::Status::invalid_argument, "violation index " + std::to_string(index) + " is out of range");
        }
        const tlor::Violation& v = list[index];
        need(out, "output pointer") =
            tlor_violation{static_cast<int32_t>(v.kind), v.day, v.route, v.point, v.message.c_str()};
    });
}

const char* tlor_violation_kind_name(int32_t kind) {
    if (kind < 0 || kind > static_cast<int32_t>(tlor::ViolationKind::max_days)) {
        return "unknown";
    }
    return tlor::to_string(static_cast<tlor::ViolationKind>(kind)).data();
}

tlor_status tlor_run_rolling(const tlor_instance* instance, const tlor_fleet* fleet, const tlor_state* initial,
                             const tlor_rolling_options* options, const double* realized_demand_kg,
                             tlor_report* report, tlor_rolling_result** out) {
    return guard([&] {
        need(out, "output pointer") = nullptr;
        const auto& inst = *need(instance, "instance").value;
        const auto& o = need(options, "options");
        tlor::RollingOptions rolling;
        rolling.first_day = o.first_day;
        rolling.last_day = o.last_day;
        rolling.horizon = o.horizon;
        rolling.step = o.step;
        rolling.schedule = {need_text(o.schedule_method, "schedule_method"), params_or_empty(o.schedule_params)};
        rolling.route = {need_text(o.route_method, "route_method"), params_or_empty(o.route_params)};
        if (o.improve_method != nullptr) {
            rolling.improve = tlor::MethodChoice{o.improve_method, params_or_empty(o.improve_params)};
        }
        rolling.weights = params_or_empty(o.weights);
        rolling.evaluation = params_or_empty(o.evaluation_params);
        rolling.check_each_step = o.check_each_step != 0;
        const auto demand = demand_of(inst, realized_demand_kg);
        auto result = std::make_shared<tlor::RollingResult>(tlor::run_rolling(
            inst, *need(fleet, "fleet").value, state_or_initial(initial, inst), rolling, demand ? &*demand : nullptr,
            report_of(report)));
        auto* handle = new tlor_rolling_result{
            result,
            tlor_solution{std::shared_ptr<tlor::Solution>(result, &result->solution), nullptr, {}},
            tlor::capi::evaluation_handle(std::shared_ptr<tlor::Evaluation>(result, &result->evaluation)),
            tlor_violations{std::shared_ptr<std::vector<tlor::Violation>>(result, &result->violations)},
            tlor_state{std::shared_ptr<tlor::State>(result, &result->end_state)},
        };
        *out = handle;
    });
}

void tlor_rolling_result_destroy(tlor_rolling_result* result) { delete result; }

const tlor_solution* tlor_rolling_result_solution(const tlor_rolling_result* result) {
    return result == nullptr ? nullptr : &result->solution;
}

const tlor_evaluation* tlor_rolling_result_evaluation(const tlor_rolling_result* result) {
    return result == nullptr ? nullptr : &result->evaluation;
}

const tlor_violations* tlor_rolling_result_violations(const tlor_rolling_result* result) {
    return result == nullptr ? nullptr : &result->violations;
}

const tlor_state* tlor_rolling_result_end_state(const tlor_rolling_result* result) {
    return result == nullptr ? nullptr : &result->end_state;
}

}
