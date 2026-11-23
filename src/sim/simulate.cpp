#include <tlor/simulate.hpp>

#include <tlor/error.hpp>

#include <string>

namespace tlor {

namespace {

std::size_t index(const SimulationResult& s, Day day, Node node) {
    return static_cast<std::size_t>(day - s.start_day) * static_cast<std::size_t>(s.node_count) +
           static_cast<std::size_t>(node);
}

}

double SimulationResult::level(Day day, Node node) const noexcept { return level_kg[index(*this, day, node)]; }

bool SimulationResult::was_collected(Day day, Node node) const noexcept { return collected[index(*this, day, node)] != 0; }

SimulationResult simulate(const Instance& instance, const State& start, Day start_day, std::int32_t days,
                          const Plan& plan, const Demand& demand) {
    const std::int32_t n = instance.node_count();
    if (start_day < 0 || days < 0 || start_day + days > demand.day_count()) {
        throw Error(Status::invalid_argument, "simulation range [" + std::to_string(start_day) + ", " +
                                                  std::to_string(start_day + days) + ") is outside the demand");
    }
    if (demand.node_count() != n) {
        throw Error(Status::invalid_argument, "demand has " + std::to_string(demand.node_count()) +
                                                  " nodes, the instance " + std::to_string(n));
    }
    validate_state(start, n, start_day);
    SimulationResult result{start_day, days, n, start, {}, {}};
    const std::size_t cells = static_cast<std::size_t>(days) * static_cast<std::size_t>(n);
    result.level_kg.resize(cells);
    result.collected.resize(cells);
    for (Day day = start_day; day < start_day + days; ++day) {
        for (Node node = 0; node < n; ++node) {
            const auto cell = index(result, day, node);
            const auto at = static_cast<std::size_t>(node);
            const double level = result.end.level_kg[at] + demand.at(day, node);
            result.level_kg[cell] = level;
            if (plan.collected(day, node)) {
                result.collected[cell] = 1;
                result.end.level_kg[at] = 0;
                result.end.last_collection[at] = day;
            } else {
                result.end.level_kg[at] = level;
            }
        }
    }
    return result;
}

std::vector<double> day_loads(const SimulationResult& simulation, const Plan& plan, Day day) {
    if (day < simulation.start_day || day >= simulation.start_day + simulation.days) {
        throw Error(Status::invalid_argument, "day " + std::to_string(day) + " was not simulated");
    }
    std::vector<double> loads;
    for (Node point : plan.points(day)) {
        if (point < 0 || point >= simulation.node_count) {
            throw Error(Status::invalid_argument, "planned node " + std::to_string(point) + " is out of range");
        }
        loads.push_back(simulation.level(day, point));
    }
    return loads;
}

}
