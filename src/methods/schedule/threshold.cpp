#include "methods/methods.hpp"

#include <cmath>
#include <optional>

namespace tlor {

namespace {

std::optional<Day> collection_day(const Fleet& fleet, Day after, Day due, Day end) {
    for (Day c = due; c > after; --c) {
        if (fleet.available_count(c) > 0) {
            return c;
        }
    }
    for (Day c = due + 1; c < end; ++c) {
        if (fleet.available_count(c) > 0) {
            return c;
        }
    }
    return std::nullopt;
}

double finite_or(const std::vector<double>* values, Node node, double fallback) {
    if (values == nullptr) {
        return fallback;
    }
    const double v = (*values)[static_cast<std::size_t>(node)];
    return std::isfinite(v) ? v : fallback;
}

}

Plan schedule_threshold(const ScheduleRequest& request) {
    const Instance& instance = request.instance;
    const Day start = request.start_day;
    const Day end = start + request.horizon;
    const double default_threshold = request.params.get_double("threshold");
    const std::vector<double>* thresholds =
        request.params.get_bool("use_point_threshold") ? instance.attribute("threshold") : nullptr;
    const std::vector<double>* max_days = instance.attribute("max_days");
    Plan plan(start, request.horizon);
    Phase phase(request.report, "due_days");
    std::int64_t collections = 0;
    for (Node point : instance.active_points()) {
        const auto at = static_cast<std::size_t>(point);
        const double limit = finite_or(thresholds, point, default_threshold) * instance.capacity(point);
        const double days_limit = finite_or(max_days, point, 0.0);
        double level = request.start_state.level_kg[at];
        Day last = request.start_state.last_collection[at];
        Day previous = start - 1;
        Day t = start;
        while (t < end) {
            const double today = level + instance.demand().at(t, point);
            const bool due = today >= limit || (days_limit > 0 && static_cast<double>(t - last) >= days_limit);
            if (!due) {
                level = today;
                ++t;
                continue;
            }
            const std::optional<Day> c = collection_day(request.fleet, previous, t, end);
            if (!c) {
                break;
            }
            plan.add(*c, point);
            ++collections;
            last = *c;
            previous = *c;
            level = 0;
            t = *c + 1;
        }
    }
    count(request.report, "collections", collections);
    return plan;
}

MethodInfo threshold_info() {
    return MethodInfo{
        "threshold",
        "Collect each point when its predicted level reaches its threshold",
        {
            {"threshold", ParamType::real, 0.85, 1e-9, 1.0, "Fill rate at which a point is due, from 0 to 1"},
            {"use_point_threshold", ParamType::boolean, true, std::nullopt, std::nullopt,
             "Use the per-point 'threshold' attribute when it is set"},
        },
        true,
    };
}

}
