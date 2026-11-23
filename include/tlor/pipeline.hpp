/**
 * @file pipeline.hpp
 * @brief Rolling-horizon driver chaining the steps over a period.
 */
#pragma once

#include <tlor/check.hpp>
#include <tlor/evaluate.hpp>
#include <tlor/registry.hpp>

#include <optional>
#include <string>
#include <vector>

namespace tlor {

/** @brief A method name and its parameters. */
struct MethodChoice {
    /** @brief Method name in the registry. */
    std::string name;
    /** @brief Its parameters. */
    Params params;
};

/** @brief Settings of a rolling-horizon run. */
struct RollingOptions {
    /** @brief First planned day. */
    Day first_day = 0;
    /** @brief Last planned day, included. */
    Day last_day = 0;
    /** @brief Days planned at each step. */
    std::int32_t horizon = 7;
    /** @brief Days kept from each plan before planning again. */
    std::int32_t step = 1;
    /** @brief Schedule method. */
    MethodChoice schedule;
    /** @brief Route method. */
    MethodChoice route;
    /** @brief Improve method, if any. */
    std::optional<MethodChoice> improve;
    /** @brief Objective weights given to the methods. */
    Params weights;
    /** @brief Evaluation parameters of the final evaluation. */
    Params evaluation;
    /** @brief Whether to check the kept days after each step and count violations in the report. */
    bool check_each_step = false;
};

/** @brief Outcome of a rolling-horizon run. */
struct RollingResult {
    /** @brief Plan and routes from first_day to last_day. */
    Solution solution;
    /** @brief Evaluation with the realized demand. */
    Evaluation evaluation;
    /** @brief Violations with the realized demand. */
    std::vector<Violation> violations;
    /** @brief State after last_day. */
    State end_state;
};

/**
 * @brief Every step days: plans horizon days, routes them, improves them if asked, keeps the first step days and
 * advances the state with the realized demand (the predicted one when realized is null).
 */
RollingResult run_rolling(const Instance& instance, const Fleet& fleet, const State& initial,
                          const RollingOptions& options, const Demand* realized, Report* report);

}
