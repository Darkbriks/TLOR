/**
 * @file steps.hpp
 * @brief One function per step: the method is chosen by name in the registry.
 *
 * Each step validates its inputs, validates the method's parameters, records a call in the report when one is
 * given, runs the method and checks the shape of its output.
 */
#pragma once

#include <tlor/registry.hpp>

#include <span>
#include <string_view>

namespace tlor {

/** @brief Plans start_day to start_day + horizon - 1, the horizon being cut at the last day of the data. */
Plan schedule(const Instance& instance, const Fleet& fleet, const State& start, Day start_day, std::int32_t horizon,
              std::string_view method, const Params& params, const Params& weights, Report* report);

/** @brief Routes the given active points of one day, loads_kg[i] being the mass to collect at points[i]. */
DayRoutes route(const Instance& instance, const Fleet& fleet, Day day, std::span<const Node> points,
                std::span<const double> loads_kg, std::string_view method, const Params& params, const Params& weights,
                Report* report);

/** @brief Improves a solution; route_method and route_params are used for any re-routing. */
Solution improve(const Instance& instance, const Fleet& fleet, const State& start, const Solution& solution,
                 std::string_view route_method, const Params& route_params, std::string_view method,
                 const Params& params, const Params& weights, Report* report);

}
