/**
 * @file check.hpp
 * @brief Independent check of a solution against the constraints of the problem.
 *
 * The checker shares no code with the methods, the simulation or the evaluation: it is built as its own target,
 * linked to the data model only.
 */
#pragma once

#include <tlor/demand.hpp>
#include <tlor/fleet.hpp>
#include <tlor/instance.hpp>
#include <tlor/solution.hpp>
#include <tlor/state.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace tlor {

/** @brief Kind of constraint a solution breaks. */
enum class ViolationKind {
    route_endpoints,
    visit_count,
    unplanned_visit,
    invalid_node,
    capacity,
    work_time,
    truck_unavailable,
    overflow,
    max_days,
};

/** @brief Name of a violation kind, such as "overflow". */
std::string_view to_string(ViolationKind kind);

/** @brief One broken constraint. */
struct Violation {
    /** @brief Kind of the violation. */
    ViolationKind kind;
    /** @brief Day it happens. */
    Day day;
    /** @brief Index of the route in its day, or -1. */
    std::int32_t route;
    /** @brief Node concerned, or -1. */
    Node point;
    /** @brief Human-readable description. */
    std::string message;
};

/**
 * @brief Every violation of a solution, with the given start state and demand.
 *
 * Levels follow the fill convention: each day the day's demand is added, then a point planned that day is emptied.
 */
std::vector<Violation> check(const Instance& instance, const Fleet& fleet, const State& start,
                             const Solution& solution, const Demand& demand);

}
