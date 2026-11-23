/**
 * @file solution.hpp
 * @brief A plan and the routes of its days.
 */
#pragma once

#include <tlor/plan.hpp>
#include <tlor/routes.hpp>

#include <optional>
#include <vector>

namespace tlor {

/** @brief A plan plus the routes of each of its days; a day may not be routed yet. */
class Solution {
public:
    /** @brief Solution with no routes. */
    explicit Solution(Plan plan);

    /** @brief The plan. */
    [[nodiscard]] const Plan& plan() const noexcept;

    /** @brief The plan, modifiable. */
    [[nodiscard]] Plan& plan() noexcept;

    /** @brief Sets the routes of their day; throws Status::invalid_argument when the day is outside the plan. */
    void set_routes(DayRoutes routes);

    /** @brief Routes of a day, or nullptr when the day is not routed or outside the plan. */
    [[nodiscard]] const DayRoutes* routes(Day day) const noexcept;

    /** @brief Forgets the routes of a day. */
    void clear_routes(Day day);

private:
    Plan plan_;
    std::vector<std::optional<DayRoutes>> routes_;
};

}
