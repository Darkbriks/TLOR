#include <tlor/evaluate.hpp>

#include <tlor/error.hpp>
#include <tlor/simulate.hpp>

#include <algorithm>
#include <limits>
#include <map>
#include <set>

namespace tlor {

namespace {

Indicators zeros() {
    Indicators indicators;
    for (std::string_view name : indicator_names) {
        indicators.values.emplace(std::string(name), 0.0);
    }
    return indicators;
}

bool in_range(const Instance& instance, Node node) { return node >= 0 && node < instance.node_count(); }

}

Indicators route_indicators(const Instance& instance, const Fleet& fleet, const DayRoutes& routes) {
    double distance = 0;
    double travel = 0;
    double service = 0;
    std::map<TruckId, double> work;
    for (const Route& route : routes.routes()) {
        double route_travel = 0;
        double route_service = 0;
        const auto& nodes = route.nodes;
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            if (!in_range(instance, nodes[i])) {
                continue;
            }
            if (i + 1 < nodes.size() && in_range(instance, nodes[i + 1])) {
                distance += instance.distance(nodes[i], nodes[i + 1]);
                route_travel += instance.duration(nodes[i], nodes[i + 1]);
            }
            if (i != 0 && i + 1 != nodes.size()) {
                route_service += instance.service(nodes[i]);
            }
        }
        if (!nodes.empty()) {
            route_service += instance.service(instance.depot());
        }
        travel += route_travel;
        service += route_service;
        work[route.truck] += route_travel + route_service;
    }
    double overtime = 0;
    for (const auto& [truck, seconds] : work) {
        if (truck >= 0 && truck < fleet.truck_count()) {
            overtime += std::max(0.0, seconds - fleet.truck(truck).max_work_s);
        }
    }
    Indicators indicators;
    indicators.values = {
        {"distance_m", distance},
        {"travel_s", travel},
        {"service_s", service},
        {"work_s", travel + service},
        {"overtime_s", overtime},
        {"routes", static_cast<double>(routes.size())},
        {"trucks_used", static_cast<double>(work.size())},
    };
    return indicators;
}

Evaluation evaluate(const Instance& instance, const Fleet& fleet, const State& start, const Solution& solution,
                    const Demand& demand, const Params& evaluation_params) {
    const double low_fill = evaluation_params.contains("low_fill_threshold")
                                ? evaluation_params.get_double("low_fill_threshold")
                                : 0.5;
    const Plan& plan = solution.plan();
    const SimulationResult sim = simulate(instance, start, plan.start(), plan.horizon(), plan, demand);
    Evaluation evaluation{plan.start(), zeros(), {}};
    double fill_sum = 0;
    double min_work = std::numeric_limits<double>::infinity();
    double max_work = -std::numeric_limits<double>::infinity();
    for (Day day = plan.start(); day < plan.end(); ++day) {
        Indicators indicators = zeros();
        if (const DayRoutes* routes = solution.routes(day)) {
            for (const auto& [name, value] : route_indicators(instance, fleet, *routes).values) {
                indicators.values[name] = value;
            }
        }
        double day_fill = 0;
        double collections = 0;
        for (Node point : plan.points(day)) {
            if (!instance.is_point(point)) {
                continue;
            }
            const double level = sim.level(day, point);
            const double fill = level / instance.capacity(point);
            collections += 1;
            indicators.values["collected_kg"] += level;
            day_fill += fill;
            if (fill < low_fill) {
                indicators.values["low_fill_collections"] += 1;
            }
        }
        indicators.values["collections"] = collections;
        indicators.values["mean_fill_at_collection"] = collections > 0 ? day_fill / collections : 0.0;
        fill_sum += day_fill;
        for (Node point = 0; point < instance.node_count(); ++point) {
            if (!instance.is_point(point)) {
                continue;
            }
            const double excess = sim.level(day, point) - instance.capacity(point);
            if (excess > 0) {
                indicators.values["overflow_point_days"] += 1;
                indicators.values["overflow_kg_days"] += excess;
            }
        }
        for (const auto& [name, value] : indicators.values) {
            if (name != "mean_fill_at_collection" && name != "workload_imbalance_s") {
                evaluation.total.values[name] += value;
            }
        }
        if (fleet.available_count(day) > 0) {
            min_work = std::min(min_work, indicators.values["work_s"]);
            max_work = std::max(max_work, indicators.values["work_s"]);
        }
        evaluation.per_day.push_back(std::move(indicators));
    }
    const double total_collections = evaluation.total.values["collections"];
    evaluation.total.values["mean_fill_at_collection"] = total_collections > 0 ? fill_sum / total_collections : 0.0;
    evaluation.total.values["workload_imbalance_s"] = max_work >= min_work ? max_work - min_work : 0.0;
    return evaluation;
}

double objective(const Indicators& indicators, const Params& weights) {
    double sum = 0;
    for (const auto& [name, value] : weights.values()) {
        if (!is_indicator_name(name)) {
            throw Error(Status::invalid_param, "unknown indicator '" + name + "' in the weights");
        }
        if (std::holds_alternative<bool>(value) || std::holds_alternative<std::string>(value)) {
            throw Error(Status::invalid_param, "weight '" + name + "' is not a number");
        }
        auto it = indicators.values.find(name);
        if (it != indicators.values.end()) {
            sum += weights.get_double(name) * it->second;
        }
    }
    return sum;
}

}
