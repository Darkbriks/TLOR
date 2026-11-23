#include <tlor/steps.hpp>

#include <tlor/error.hpp>
#include <tlor/evaluate.hpp>

#include <algorithm>
#include <cmath>
#include <set>
#include <string>

namespace tlor {

namespace {

[[noreturn]] void invalid(const std::string& message) { throw Error(Status::invalid_argument, message); }

void check_common(const Instance& instance, const Fleet& fleet, const Params& weights) {
    if (fleet.day_count() != instance.day_count()) {
        invalid("the fleet covers " + std::to_string(fleet.day_count()) + " days, the instance " +
                std::to_string(instance.day_count()));
    }
    (void)objective(Indicators{}, weights);
}

void check_day(const Instance& instance, Day day) {
    if (day < 0 || day >= instance.day_count()) {
        invalid("day " + std::to_string(day) + " is outside the data");
    }
}

InstanceSize size_of(const Instance& instance, const Fleet& fleet) {
    return {instance.node_count(), instance.active_point_count(), instance.day_count(), fleet.truck_count()};
}

template <class Run, class Measure>
auto call(Report* report, CallInfo info, Run run, Measure measure) {
    const bool top = report != nullptr && !report->in_call();
    if (report != nullptr) {
        report->begin_call(std::move(info));
    }
    try {
        auto result = run();
        if (report != nullptr) {
            report->end_call(Status::ok, top ? std::optional<Indicators>(measure(result)) : std::nullopt);
        }
        return result;
    } catch (const Error& e) {
        if (report != nullptr && report->in_call()) {
            report->end_call(e.status(), std::nullopt);
        }
        throw;
    } catch (...) {
        if (report != nullptr && report->in_call()) {
            report->end_call(Status::internal_error, std::nullopt);
        }
        throw;
    }
}

[[noreturn]] void bad_output(std::string_view step, std::string_view method, const std::string& why) {
    throw Error(Status::internal_error, std::string(step) + " method '" + std::string(method) + "' " + why);
}

}

Plan schedule(const Instance& instance, const Fleet& fleet, const State& start, Day start_day, std::int32_t horizon,
              std::string_view method, const Params& params, const Params& weights, Report* report) {
    check_common(instance, fleet, weights);
    check_day(instance, start_day);
    if (horizon < 1) {
        invalid("the horizon must be at least one day");
    }
    horizon = std::min(horizon, instance.day_count() - start_day);
    validate_state(start, instance.node_count(), start_day);
    const Registry& registry = Registry::global();
    const MethodInfo& info = registry.info(Step::schedule, method);
    const Params validated = validate_params(info, params);
    const ScheduleFn& fn = registry.schedule_fn(method);
    return call(
        report, CallInfo{"schedule", info.name, validated, size_of(instance, fleet)},
        [&] {
            Plan plan = fn(ScheduleRequest{instance, fleet, start, start_day, horizon, validated, weights, report});
            if (plan.start() != start_day || plan.horizon() != horizon) {
                bad_output("schedule", method, "returned a plan of another range");
            }
            return plan;
        },
        [&](const Plan& plan) { return evaluate(instance, fleet, start, Solution(plan), instance.demand(), {}).total; });
}

DayRoutes route(const Instance& instance, const Fleet& fleet, Day day, std::span<const Node> points,
                std::span<const double> loads_kg, std::string_view method, const Params& params, const Params& weights,
                Report* report) {
    check_common(instance, fleet, weights);
    check_day(instance, day);
    if (points.size() != loads_kg.size()) {
        invalid(std::to_string(points.size()) + " points but " + std::to_string(loads_kg.size()) + " loads");
    }
    std::set<Node> seen;
    for (std::size_t i = 0; i < points.size(); ++i) {
        if (!instance.is_point(points[i])) {
            invalid("node " + std::to_string(points[i]) + " is not an active collection point");
        }
        if (!seen.insert(points[i]).second) {
            invalid("point " + std::to_string(points[i]) + " appears twice");
        }
        if (!std::isfinite(loads_kg[i]) || loads_kg[i] < 0) {
            invalid("the load of point " + std::to_string(points[i]) + " is not a finite non-negative number");
        }
    }
    const Registry& registry = Registry::global();
    const MethodInfo& info = registry.info(Step::route, method);
    const Params validated = validate_params(info, params);
    const RouteFn& fn = registry.route_fn(method);
    return call(
        report, CallInfo{"route", info.name, validated, size_of(instance, fleet)},
        [&] {
            DayRoutes routes = fn(RouteRequest{instance, fleet, day, points, loads_kg, validated, weights, report});
            if (routes.day() != day) {
                bad_output("route", method, "returned routes of another day");
            }
            return routes;
        },
        [&](const DayRoutes& routes) { return route_indicators(instance, fleet, routes); });
}

Solution improve(const Instance& instance, const Fleet& fleet, const State& start, const Solution& solution,
                 std::string_view route_method, const Params& route_params, std::string_view method,
                 const Params& params, const Params& weights, Report* report) {
    check_common(instance, fleet, weights);
    const Plan& plan = solution.plan();
    if (plan.end() > instance.day_count()) {
        invalid("the solution's plan runs past the last day of the data");
    }
    validate_state(start, instance.node_count(), plan.start());
    const Registry& registry = Registry::global();
    (void)validate_params(registry.info(Step::route, route_method), route_params);
    const MethodInfo& info = registry.info(Step::improve, method);
    const Params validated = validate_params(info, params);
    const ImproveFn& fn = registry.improve_fn(method);
    return call(
        report, CallInfo{"improve", info.name, validated, size_of(instance, fleet)},
        [&] {
            Solution result = fn(ImproveRequest{instance, fleet, start, solution, route_method, route_params, validated,
                                                weights, report});
            if (result.plan().start() != plan.start() || result.plan().horizon() != plan.horizon()) {
                bad_output("improve", method, "returned a plan of another range");
            }
            return result;
        },
        [&](const Solution& result) { return evaluate(instance, fleet, start, result, instance.demand(), {}).total; });
}

}
