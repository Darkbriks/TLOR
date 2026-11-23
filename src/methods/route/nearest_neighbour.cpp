#include "methods/methods.hpp"

#include <tlor/error.hpp>

#include <limits>
#include <optional>

namespace tlor {

namespace {

class Builder {
public:
    explicit Builder(const RouteRequest& request)
        : request_(request), instance_(request.instance), depot_(request.instance.depot()),
          visited_(request.points.size(), false), work_(static_cast<std::size_t>(request.fleet.truck_count()), 0.0),
          routes_(request.day) {
        const std::string& criterion = request.params.get_string("criterion");
        if (criterion != "distance" && criterion != "duration") {
            throw Error(Status::invalid_param, "criterion must be 'distance' or 'duration', not '" + criterion + "'");
        }
        by_distance_ = criterion == "distance";
        remaining_ = request.points.size();
    }

    DayRoutes run() {
        for (TruckId truck : request_.fleet.available_trucks(request_.day)) {
            while (remaining_ > 0 && build(truck, true)) {
            }
        }
        std::vector<TruckId> fallback = request_.fleet.available_trucks(request_.day);
        if (fallback.empty()) {
            for (TruckId t = 0; t < request_.fleet.truck_count(); ++t) {
                fallback.push_back(t);
            }
        }
        while (remaining_ > 0) {
            TruckId best = fallback.front();
            for (TruckId t : fallback) {
                if (work_[static_cast<std::size_t>(t)] < work_[static_cast<std::size_t>(best)]) {
                    best = t;
                }
            }
            build(best, false);
        }
        count(request_.report, "candidates_evaluated", candidates_);
        return std::move(routes_);
    }

private:
    double cost(Node from, Node to) const {
        return by_distance_ ? instance_.distance(from, to) : instance_.duration(from, to);
    }

    double closing(Node node) const { return instance_.duration(node, depot_) + instance_.service(depot_); }

    std::optional<std::size_t> nearest(Node from, double load, double time, const Truck& truck, bool timed,
                                       bool force) {
        std::optional<std::size_t> best;
        double best_cost = std::numeric_limits<double>::infinity();
        for (std::size_t i = 0; i < request_.points.size(); ++i) {
            if (visited_[i]) {
                continue;
            }
            ++candidates_;
            const Node p = request_.points[i];
            const bool fits = force || load + request_.loads_kg[i] <= truck.capacity_kg;
            const bool in_time =
                !timed || time + instance_.duration(from, p) + instance_.service(p) + closing(p) <= truck.max_work_s;
            if (!fits || !in_time) {
                continue;
            }
            const double c = cost(from, p);
            if (c < best_cost || (c == best_cost && best && p < request_.points[*best])) {
                best = i;
                best_cost = c;
            }
        }
        return best;
    }

    bool build(TruckId truck_id, bool timed) {
        const Truck& truck = request_.fleet.truck(truck_id);
        Route route{truck_id, {depot_}};
        Node current = depot_;
        double load = 0;
        double time = work_[static_cast<std::size_t>(truck_id)];
        while (true) {
            const bool first = route.nodes.size() == 1;
            const auto next = nearest(current, load, time, truck, timed, first && !timed);
            if (!next) {
                break;
            }
            const Node p = request_.points[*next];
            visited_[*next] = true;
            --remaining_;
            load += request_.loads_kg[*next];
            time += instance_.duration(current, p) + instance_.service(p);
            route.nodes.push_back(p);
            current = p;
        }
        if (route.nodes.size() == 1) {
            return false;
        }
        time += closing(current);
        route.nodes.push_back(depot_);
        work_[static_cast<std::size_t>(truck_id)] = time;
        routes_.add(std::move(route));
        return true;
    }

    const RouteRequest& request_;
    const Instance& instance_;
    Node depot_;
    bool by_distance_ = true;
    std::vector<bool> visited_;
    std::size_t remaining_ = 0;
    std::vector<double> work_;
    DayRoutes routes_;
    std::int64_t candidates_ = 0;
};

}

DayRoutes route_nearest_neighbour(const RouteRequest& request) {
    Builder builder(request);
    Phase phase(request.report, "construction");
    return builder.run();
}

MethodInfo nearest_neighbour_info() {
    return MethodInfo{
        "nearest_neighbour",
        "Greedy routes to the nearest point that fits in the truck's capacity and work time",
        {
            {"criterion", ParamType::string, std::string("distance"), std::nullopt, std::nullopt,
             "'distance' or 'duration': what 'nearest' means"},
        },
        true,
    };
}

}
