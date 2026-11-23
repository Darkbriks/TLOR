#include <tlor/solution.hpp>

#include <tlor/error.hpp>

#include <string>

namespace tlor {

Solution::Solution(Plan plan) : plan_(std::move(plan)) {
    routes_.resize(static_cast<std::size_t>(plan_.horizon()));
}

const Plan& Solution::plan() const noexcept { return plan_; }

Plan& Solution::plan() noexcept { return plan_; }

void Solution::set_routes(DayRoutes routes) {
    const Day day = routes.day();
    if (!plan_.contains(day)) {
        throw Error(Status::invalid_argument, "day " + std::to_string(day) + " is outside the plan");
    }
    routes_[static_cast<std::size_t>(day - plan_.start())] = std::move(routes);
}

const DayRoutes* Solution::routes(Day day) const noexcept {
    if (!plan_.contains(day)) {
        return nullptr;
    }
    const auto& slot = routes_[static_cast<std::size_t>(day - plan_.start())];
    return slot ? &*slot : nullptr;
}

void Solution::clear_routes(Day day) {
    if (plan_.contains(day)) {
        routes_[static_cast<std::size_t>(day - plan_.start())].reset();
    }
}

}
