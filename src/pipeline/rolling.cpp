#include <tlor/pipeline.hpp>

#include <tlor/error.hpp>
#include <tlor/simulate.hpp>
#include <tlor/steps.hpp>

#include <string>

namespace tlor {

namespace {

[[noreturn]] void invalid(const std::string& message) { throw Error(Status::invalid_argument, message); }

void validate(const Instance& instance, const Fleet& fleet, const State& initial, const RollingOptions& o,
              const Demand* realized) {
    if (o.first_day < 0 || o.last_day < o.first_day || o.last_day >= instance.day_count()) {
        invalid("rolling range [" + std::to_string(o.first_day) + ", " + std::to_string(o.last_day) +
                "] is outside the data");
    }
    if (o.step < 1 || o.horizon < o.step) {
        invalid("rolling needs step >= 1 and horizon >= step");
    }
    if (realized != nullptr &&
        (realized->day_count() != instance.day_count() || realized->node_count() != instance.node_count())) {
        invalid("the realized demand does not have the instance's days and nodes");
    }
    if (fleet.day_count() != instance.day_count()) {
        invalid("the fleet does not cover the instance's days");
    }
    validate_state(initial, instance.node_count(), o.first_day);
}

std::string method_label(const RollingOptions& o) {
    std::string label = o.schedule.name + "+" + o.route.name;
    if (o.improve) {
        label += "+" + o.improve->name;
    }
    return label;
}

Params recorded(const RollingOptions& o) {
    Params p;
    p.set("first_day", std::int64_t{o.first_day});
    p.set("last_day", std::int64_t{o.last_day});
    p.set("horizon", std::int64_t{o.horizon});
    p.set("step", std::int64_t{o.step});
    p.set("check_each_step", o.check_each_step);
    return p;
}

class Rolling {
public:
    Rolling(const Instance& instance, const Fleet& fleet, const State& initial, const RollingOptions& options,
            const Demand& actual, Report* report)
        : instance_(instance), fleet_(fleet), initial_(initial), o_(options), actual_(actual), report_(report),
          global_(Plan(options.first_day, options.last_day - options.first_day + 1)), state_(initial) {}

    RollingResult run() {
        for (Day d = o_.first_day; d <= o_.last_day; d += o_.step) {
            window(d);
        }
        std::vector<Violation> violations;
        {
            Phase phase(report_, "check");
            violations = check(instance_, fleet_, initial_, global_, actual_);
        }
        Evaluation evaluation;
        {
            Phase phase(report_, "evaluate");
            evaluation = evaluate(instance_, fleet_, initial_, global_, actual_, o_.evaluation);
        }
        return RollingResult{std::move(global_), std::move(evaluation), std::move(violations), std::move(state_)};
    }

private:
    void window(Day d) {
        const std::int32_t horizon = std::min(o_.horizon, o_.last_day + 1 - d);
        const std::int32_t keep = std::min(o_.step, horizon);
        std::optional<Solution> solution;
        {
            Phase phase(report_, "schedule");
            solution.emplace(schedule(instance_, fleet_, state_, d, horizon, o_.schedule.name, o_.schedule.params,
                                      o_.weights, report_));
        }
        {
            Phase phase(report_, "route");
            const Plan& plan = solution->plan();
            const SimulationResult sim = simulate(instance_, state_, d, horizon, plan, instance_.demand());
            const std::int32_t routed = o_.improve ? horizon : keep;
            for (Day day = d; day < d + routed; ++day) {
                const auto points = plan.points(day);
                if (points.empty()) {
                    solution->set_routes(DayRoutes(day));
                    continue;
                }
                const std::vector<double> loads = day_loads(sim, plan, day);
                solution->set_routes(route(instance_, fleet_, day, points, loads, o_.route.name, o_.route.params,
                                           o_.weights, report_));
            }
        }
        if (o_.improve) {
            Phase phase(report_, "improve");
            solution.emplace(improve(instance_, fleet_, state_, *solution, o_.route.name, o_.route.params,
                                     o_.improve->name, o_.improve->params, o_.weights, report_));
        }
        Solution kept(Plan(d, keep));
        for (Day day = d; day < d + keep; ++day) {
            for (Node point : solution->plan().points(day)) {
                global_.plan().add(day, point);
                kept.plan().add(day, point);
            }
            if (const DayRoutes* routes = solution->routes(day)) {
                global_.set_routes(*routes);
                kept.set_routes(*routes);
            }
        }
        if (o_.check_each_step) {
            Phase phase(report_, "check");
            for (const Violation& v : check(instance_, fleet_, state_, kept, actual_)) {
                count(report_, "violations:" + std::string(to_string(v.kind)));
            }
        }
        Phase phase(report_, "simulate");
        state_ = simulate(instance_, state_, d, keep, kept.plan(), actual_).end;
    }

    const Instance& instance_;
    const Fleet& fleet_;
    const State& initial_;
    const RollingOptions& o_;
    const Demand& actual_;
    Report* report_;
    Solution global_;
    State state_;
};

}

RollingResult run_rolling(const Instance& instance, const Fleet& fleet, const State& initial,
                          const RollingOptions& options, const Demand* realized, Report* report) {
    validate(instance, fleet, initial, options, realized);
    const Demand& actual = realized != nullptr ? *realized : instance.demand();
    if (report != nullptr) {
        report->begin_call(CallInfo{"rolling", method_label(options), recorded(options),
                                    {instance.node_count(), instance.active_point_count(), instance.day_count(),
                                     fleet.truck_count()}});
    }
    try {
        RollingResult result = Rolling(instance, fleet, initial, options, actual, report).run();
        if (report != nullptr) {
            report->end_call(Status::ok, result.evaluation.total);
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

}
