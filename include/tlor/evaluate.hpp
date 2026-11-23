/**
 * @file evaluate.hpp
 * @brief Indicators of a solution and the weighted objective built from them.
 */
#pragma once

#include <tlor/demand.hpp>
#include <tlor/fleet.hpp>
#include <tlor/indicators.hpp>
#include <tlor/instance.hpp>
#include <tlor/params.hpp>
#include <tlor/routes.hpp>
#include <tlor/solution.hpp>
#include <tlor/state.hpp>

#include <vector>

namespace tlor {

/** @brief Indicators of a solution, per day and in total. */
struct Evaluation {
    /** @brief First day of the evaluated plan. */
    Day start_day = 0;
    /** @brief Indicators over the whole plan. */
    Indicators total;
    /** @brief Indicators of each day of the plan. */
    std::vector<Indicators> per_day;
};

/** @brief distance_m, travel_s, service_s, work_s, overtime_s, routes and trucks_used of one day's routes. */
Indicators route_indicators(const Instance& instance, const Fleet& fleet, const DayRoutes& routes);

/**
 * @brief Every indicator of a solution, for each day of its plan and in total.
 *
 * evaluation_params: low_fill_threshold (default 0.5), below which a collection counts as low-fill.
 */
Evaluation evaluate(const Instance& instance, const Fleet& fleet, const State& start, const Solution& solution,
                    const Demand& demand, const Params& evaluation_params);

/** @brief Sum of weight times indicator; throws Status::invalid_param on an unknown name or a non-numeric weight. */
double objective(const Indicators& indicators, const Params& weights);

}
