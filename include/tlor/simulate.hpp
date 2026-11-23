/**
 * @file simulate.hpp
 * @brief Container levels day by day under a plan.
 */
#pragma once

#include <tlor/demand.hpp>
#include <tlor/instance.hpp>
#include <tlor/plan.hpp>
#include <tlor/state.hpp>

#include <cstdint>
#include <vector>

namespace tlor {

/** @brief Levels of every node over the simulated days, and the state after them. */
struct SimulationResult {
    /** @brief First simulated day. */
    Day start_day = 0;
    /** @brief Number of simulated days. */
    std::int32_t days = 0;
    /** @brief Number of nodes. */
    std::int32_t node_count = 0;
    /** @brief State at the start of the day after the last simulated one. */
    State end;
    /** @brief Level before collection, [(day - start_day) * node_count + node], in kilograms. */
    std::vector<double> level_kg;
    /** @brief Whether the node was collected that day, same layout. */
    std::vector<std::uint8_t> collected;

    /** @brief Level of a node on a day, before collection. */
    [[nodiscard]] double level(Day day, Node node) const noexcept;

    /** @brief Whether a node was collected on a day. */
    [[nodiscard]] bool was_collected(Day day, Node node) const noexcept;
};

/**
 * @brief Simulates the levels from start_day for the given number of days.
 *
 * Each day the day's demand is added first; a node collected that day (day in the plan and node in its points)
 * is then emptied. Throws Status::invalid_argument on a negative range, a range past the demand, or a state of
 * the wrong size.
 */
SimulationResult simulate(const Instance& instance, const State& start, Day start_day, std::int32_t days,
                          const Plan& plan, const Demand& demand);

/** @brief Collected mass of each point of plan.points(day), in the same order. */
std::vector<double> day_loads(const SimulationResult& simulation, const Plan& plan, Day day);

}
