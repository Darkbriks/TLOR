#include "methods/methods.hpp"

#include <tlor/report.hpp>

#include <limits>
#include <optional>
#include <vector>

namespace tlor {

namespace {

class Greedy {
public:
    explicit Greedy(const RouteRequest& request)
        : request_(request), depot_(request.instance.depot()), visited_(request.points.size(), false),
          routes_(request.day) {
        const std::vector<TruckId> available = request.fleet.available_trucks(request.day);
        truck_ = available.empty() ? 0 : available.front();
        capacity_ = request.fleet.truck(truck_).capacity_kg;
    }

    DayRoutes compute() {
        Phase phase(request_.report, "construction");
        std::size_t remaining = request_.points.size();
        while (remaining > 0) {
            std::optional<std::size_t> next = nearest(true);
            if (!next) {
                if (position_ != depot_) {
                    close_tour();
                    continue;
                }
                next = nearest(false);
            }
            visit(*next);
            --remaining;
        }
        if (position_ != depot_) {
            close_tour();
        }
        return std::move(routes_);
    }

private:
    std::optional<std::size_t> nearest(bool must_fit) const {
        std::optional<std::size_t> best;
        double minimum = std::numeric_limits<double>::infinity();
        for (std::size_t i = 0; i < request_.points.size(); ++i) {
            if (visited_[i] || (must_fit && load_ + request_.loads_kg[i] > capacity_)) {
                continue;
            }
            const double d = request_.instance.distance(position_, request_.points[i]);
            if (d < minimum || (d == minimum && best && request_.points[i] < request_.points[*best])) {
                minimum = d;
                best = i;
            }
        }
        return best;
    }

    void visit(std::size_t index) {
        visited_[index] = true;
        load_ += request_.loads_kg[index];
        position_ = request_.points[index];
        tour_.push_back(position_);
    }

    void close_tour() {
        tour_.push_back(depot_);
        routes_.add(Route{truck_, std::move(tour_)});
        tour_ = {depot_};
        position_ = depot_;
        load_ = 0;
    }

    const RouteRequest& request_;
    Node depot_;
    std::vector<bool> visited_;
    DayRoutes routes_;
    TruckId truck_ = 0;
    double capacity_ = 0;
    std::vector<Node> tour_{depot_};
    Node position_ = depot_;
    double load_ = 0;
};

}

DayRoutes route_greedy(const RouteRequest& request) { return Greedy(request).compute(); }

MethodInfo greedy_info() {
    return MethodInfo{
        "greedy",
        "Nearest point that fits in the first available truck, back to the depot when it is full; no time limit",
        {},
        true,
    };
}

}
