#include "methods/methods.hpp"

#include <tlor/evaluate.hpp>
#include <tlor/random.hpp>
#include <tlor/simulate.hpp>
#include <tlor/steps.hpp>

#include <algorithm>
#include <map>

namespace tlor {

namespace {

struct Move {
    Day from;
    Node point;
    Day to;
};

using Loads = std::map<Day, std::vector<double>>;

class Search {
public:
    explicit Search(const ImproveRequest& request) : r_(request), current_(request.solution) {}

    Solution run() {
        {
            Phase phase(r_.report, "initial_routing");
            loads_ = loads_of(current_.plan());
            for (Day day = plan().start(); day < plan().end(); ++day) {
                if (!plan().points(day).empty() && current_.routes(day) == nullptr) {
                    route_day(current_, day, loads_.at(day));
                }
            }
        }
        Phase phase(r_.report, "search");
        count(r_.report, "moves_evaluated", 0);
        count(r_.report, "moves_accepted", 0);
        const std::int64_t max_iterations = r_.params.get_int("max_iterations");
        const auto seed = static_cast<std::uint64_t>(r_.params.get_int("seed"));
        double best = score(current_);
        std::int64_t evaluated = 0;
        for (std::uint64_t pass = 0; evaluated < max_iterations; ++pass) {
            std::vector<Move> moves = candidates();
            Random(seed + pass).shuffle(std::span(moves));
            bool accepted = false;
            for (const Move& move : moves) {
                if (evaluated >= max_iterations) {
                    break;
                }
                ++evaluated;
                count(r_.report, "moves_evaluated");
                Solution trial = current_;
                trial.plan().remove(move.from, move.point);
                trial.plan().add(move.to, move.point);
                Loads trial_loads = loads_of(trial.plan());
                for (Day day = plan().start(); day < plan().end(); ++day) {
                    const bool same_points = std::ranges::equal(plan().points(day), trial.plan().points(day));
                    if (!same_points || loads_.at(day) != trial_loads.at(day)) {
                        route_day(trial, day, trial_loads.at(day));
                    }
                }
                const double value = score(trial);
                if (value < best) {
                    best = value;
                    current_ = std::move(trial);
                    loads_ = std::move(trial_loads);
                    count(r_.report, "moves_accepted");
                    accepted = true;
                    break;
                }
            }
            if (!accepted) {
                break;
            }
        }
        return std::move(current_);
    }

private:
    const Plan& plan() const { return current_.plan(); }

    Loads loads_of(const Plan& p) const {
        const SimulationResult sim =
            simulate(r_.instance, r_.start_state, p.start(), p.horizon(), p, r_.instance.demand());
        Loads loads;
        for (Day day = p.start(); day < p.end(); ++day) {
            loads.emplace(day, day_loads(sim, p, day));
        }
        return loads;
    }

    void route_day(Solution& solution, Day day, const std::vector<double>& loads) const {
        const auto points = solution.plan().points(day);
        if (points.empty()) {
            solution.set_routes(DayRoutes(day));
            return;
        }
        solution.set_routes(route(r_.instance, r_.fleet, day, points, loads, r_.route_method, r_.route_params,
                                  r_.weights, r_.report));
    }

    double score(const Solution& solution) const {
        return objective(evaluate(r_.instance, r_.fleet, r_.start_state, solution, r_.instance.demand(), {}).total,
                         r_.weights);
    }

    std::vector<Move> candidates() const {
        const std::int64_t max_shift = r_.params.get_int("max_shift");
        std::vector<Move> moves;
        for (Day day = plan().start(); day < plan().end(); ++day) {
            for (Node point : plan().points(day)) {
                for (std::int64_t delta = -max_shift; delta <= max_shift; ++delta) {
                    const auto target = static_cast<Day>(day + delta);
                    if (delta == 0 || !plan().contains(target) || r_.fleet.available_count(target) == 0 ||
                        plan().collected(target, point)) {
                        continue;
                    }
                    moves.push_back({day, point, target});
                }
            }
        }
        return moves;
    }

    const ImproveRequest& r_;
    Solution current_;
    Loads loads_;
};

}

Solution improve_shift_day(const ImproveRequest& request) { return Search(request).run(); }

MethodInfo shift_day_info() {
    return MethodInfo{
        "shift_day",
        "Local search moving one collection to a nearby day when it lowers the objective",
        {
            {"max_iterations", ParamType::integer, std::int64_t{1000}, 0.0, std::nullopt, "Maximum number of moves evaluated"},
            {"max_shift", ParamType::integer, std::int64_t{1}, 1.0, std::nullopt, "Largest shift of a collection, in days"},
            {"seed", ParamType::integer, std::int64_t{0}, 0.0, std::nullopt, "Seed of the order in which moves are tried"},
        },
        true,
    };
}

}
