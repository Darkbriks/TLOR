#include <tlor/state.hpp>

#include <tlor/error.hpp>

#include <cmath>
#include <string>

namespace tlor {

State State::initial(std::int32_t node_count) {
    const auto n = static_cast<std::size_t>(node_count < 0 ? 0 : node_count);
    return State{std::vector<double>(n, 0.0), std::vector<Day>(n, -1)};
}

void validate_state(const State& state, std::int32_t node_count, Day start_day) {
    const auto n = static_cast<std::size_t>(node_count < 0 ? 0 : node_count);
    if (state.level_kg.size() != n || state.last_collection.size() != n) {
        throw Error(Status::invalid_argument, "state has " + std::to_string(state.level_kg.size()) + " levels and " +
                                                  std::to_string(state.last_collection.size()) +
                                                  " last collections, expected " + std::to_string(n));
    }
    for (std::size_t node = 0; node < n; ++node) {
        const double level = state.level_kg[node];
        if (!std::isfinite(level) || level < 0) {
            throw Error(Status::invalid_argument, "state level of node " + std::to_string(node) + " is not a finite non-negative number");
        }
        const Day last = state.last_collection[node];
        if (last < -1 || last >= start_day) {
            throw Error(Status::invalid_argument, "last collection of node " + std::to_string(node) + " is day " +
                                                      std::to_string(last) + ", expected -1 or a day before " +
                                                      std::to_string(start_day));
        }
    }
}

}
