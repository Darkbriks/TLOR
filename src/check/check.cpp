#include <tlor/check.hpp>

#include <tlor/error.hpp>

#include <cmath>
#include <map>
#include <set>

namespace tlor {

std::string_view to_string(ViolationKind kind) {
    switch (kind) {
        case ViolationKind::route_endpoints: return "route_endpoints";
        case ViolationKind::visit_count: return "visit_count";
        case ViolationKind::unplanned_visit: return "unplanned_visit";
        case ViolationKind::invalid_node: return "invalid_node";
        case ViolationKind::capacity: return "capacity";
        case ViolationKind::work_time: return "work_time";
        case ViolationKind::truck_unavailable: return "truck_unavailable";
        case ViolationKind::overflow: return "overflow";
        case ViolationKind::max_days: return "max_days";
    }
    return "unknown";
}

namespace {

class Checker {
public:
    Checker(const Instance& instance, const Fleet& fleet, const State& start, const Solution& solution,
            const Demand& demand)
        : instance_(instance), fleet_(fleet), solution_(solution), demand_(demand), level_(start.level_kg),
          last_(start.last_collection) {
        const auto n = static_cast<std::size_t>(instance.node_count());
        const Plan& plan = solution.plan();
        validate_state(start, instance.node_count(), plan.start());
        if (demand.node_count() != instance.node_count() || plan.end() > demand.day_count()) {
            throw Error(Status::invalid_argument, "demand does not cover the instance's nodes and the plan's days");
        }
        const std::vector<double>* max_days = instance.attribute("max_days");
        if (max_days != nullptr) {
            max_days_ = *max_days;
        }
        flagged_.assign(n, false);
    }

    std::vector<Violation> run() {
        const Plan& plan = solution_.plan();
        for (Day day = plan.start(); day < plan.end(); ++day) {
            std::map<Node, double> collected = advance(day);
            check_day(day, collected);
        }
        return std::move(violations_);
    }

private:
    void add(ViolationKind kind, Day day, std::int32_t route, Node point, std::string message) {
        violations_.push_back({kind, day, route, point, std::move(message)});
    }

    bool in_range(Node node) const { return node >= 0 && node < instance_.node_count(); }

    std::map<Node, double> advance(Day day) {
        const Plan& plan = solution_.plan();
        std::map<Node, double> collected;
        for (Node node = 0; node < instance_.node_count(); ++node) {
            const auto at = static_cast<std::size_t>(node);
            const double level = level_[at] + demand_.at(day, node);
            const bool planned = plan.collected(day, node);
            if (instance_.is_point(node)) {
                if (level > instance_.capacity(node)) {
                    add(ViolationKind::overflow, day, -1, node,
                        "point " + std::to_string(node) + " holds " + std::to_string(level) + " kg for a capacity of " +
                            std::to_string(instance_.capacity(node)) + " kg");
                }
                check_max_days(day, node, planned);
            }
            if (planned) {
                collected[node] = level;
                level_[at] = 0;
                last_[at] = day;
                flagged_[at] = false;
            } else {
                level_[at] = level;
            }
        }
        return collected;
    }

    void check_max_days(Day day, Node node, bool planned) {
        if (max_days_.empty()) {
            return;
        }
        const auto at = static_cast<std::size_t>(node);
        const double limit = max_days_[at];
        if (!std::isfinite(limit) || limit <= 0 || flagged_[at]) {
            return;
        }
        if (static_cast<double>(day - last_[at]) > limit) {
            add(ViolationKind::max_days, day, -1, node,
                "point " + std::to_string(node) + " went more than " + std::to_string(limit) + " days without collection");
            flagged_[at] = !planned;
        }
    }

    void check_day(Day day, const std::map<Node, double>& collected) {
        const Plan& plan = solution_.plan();
        std::map<Node, int> visits;
        for (Node point : plan.points(day)) {
            if (!instance_.is_point(point)) {
                add(ViolationKind::invalid_node, day, -1, point,
                    "planned node " + std::to_string(point) + " is not an active collection point");
            } else {
                visits[point] = 0;
            }
        }
        std::map<TruckId, double> work;
        const DayRoutes* routes = solution_.routes(day);
        if (routes != nullptr) {
            std::int32_t index = 0;
            for (const Route& route : routes->routes()) {
                check_route(day, index, route, collected, visits, work);
                ++index;
            }
        }
        for (const auto& [point, count] : visits) {
            if (count != 1) {
                add(ViolationKind::visit_count, day, -1, point,
                    "planned point " + std::to_string(point) + " is visited " + std::to_string(count) + " times");
            }
        }
        for (const auto& [truck, seconds] : work) {
            if (seconds > fleet_.truck(truck).max_work_s) {
                add(ViolationKind::work_time, day, -1, -1,
                    "truck " + std::to_string(truck) + " works " + std::to_string(seconds) + " s for a limit of " +
                        std::to_string(fleet_.truck(truck).max_work_s) + " s");
            }
        }
    }

    void check_route(Day day, std::int32_t index, const Route& route, const std::map<Node, double>& collected,
                     std::map<Node, int>& visits, std::map<TruckId, double>& work) {
        const Node depot = instance_.depot();
        const auto& nodes = route.nodes;
        if (nodes.size() < 2 || nodes.front() != depot || nodes.back() != depot) {
            add(ViolationKind::route_endpoints, day, index, -1, "route does not start and end at the depot");
        }
        std::set<Node> distinct;
        double seconds = 0;
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            const Node node = nodes[i];
            const bool endpoint = i == 0 || i + 1 == nodes.size();
            if (!in_range(node)) {
                add(ViolationKind::invalid_node, day, index, node, "node " + std::to_string(node) + " is out of range");
                continue;
            }
            if (i + 1 < nodes.size() && in_range(nodes[i + 1])) {
                seconds += instance_.duration(node, nodes[i + 1]);
            }
            if (endpoint) {
                continue;
            }
            seconds += instance_.service(node);
            if (node == depot) {
                add(ViolationKind::route_endpoints, day, index, node, "route goes through the depot");
                continue;
            }
            if (!instance_.is_point(node)) {
                add(ViolationKind::invalid_node, day, index, node,
                    "node " + std::to_string(node) + " is not an active collection point");
                continue;
            }
            auto visit = visits.find(node);
            if (visit == visits.end()) {
                add(ViolationKind::unplanned_visit, day, index, node,
                    "point " + std::to_string(node) + " is visited but not planned");
                continue;
            }
            ++visit->second;
            distinct.insert(node);
        }
        if (!nodes.empty() && nodes.back() == depot) {
            seconds += instance_.service(depot);
        }
        if (route.truck < 0 || route.truck >= fleet_.truck_count()) {
            add(ViolationKind::truck_unavailable, day, index, -1, "truck " + std::to_string(route.truck) + " does not exist");
            return;
        }
        if (!fleet_.available(day, route.truck)) {
            add(ViolationKind::truck_unavailable, day, index, -1,
                "truck " + std::to_string(route.truck) + " is not available on day " + std::to_string(day));
        }
        double load = 0;
        for (Node node : distinct) {
            load += collected.at(node);
        }
        const double capacity = fleet_.truck(route.truck).capacity_kg;
        if (load > capacity) {
            add(ViolationKind::capacity, day, index, -1,
                "route carries " + std::to_string(load) + " kg for a capacity of " + std::to_string(capacity) + " kg");
        }
        work[route.truck] += seconds;
    }

    const Instance& instance_;
    const Fleet& fleet_;
    const Solution& solution_;
    const Demand& demand_;
    std::vector<double> level_;
    std::vector<Day> last_;
    std::vector<double> max_days_;
    std::vector<bool> flagged_;
    std::vector<Violation> violations_;
};

}

std::vector<Violation> check(const Instance& instance, const Fleet& fleet, const State& start,
                             const Solution& solution, const Demand& demand) {
    return Checker(instance, fleet, start, solution, demand).run();
}

}
