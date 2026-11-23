/**
 * @file state.hpp
 * @brief Content of every container at the start of a day.
 */
#pragma once

#include <tlor/types.hpp>

#include <cstdint>
#include <vector>

namespace tlor {

/** @brief Waste in each container at the start of a day, and the last day each was collected. */
struct State {
    /** @brief Waste in kilograms, per node. */
    std::vector<double> level_kg;
    /** @brief Last collection day, per node; -1 when never collected. */
    std::vector<Day> last_collection;

    /** @brief Empty containers, never collected. */
    static State initial(std::int32_t node_count);
};

/**
 * @brief Throws Status::invalid_argument unless the state has node_count entries, finite non-negative levels and
 * last collections in [-1, start_day).
 */
void validate_state(const State& state, std::int32_t node_count, Day start_day);

}
