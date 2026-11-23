#include <tlor/solution.hpp>

#include <tlor/error.hpp>

#include <string>

namespace tlor {

Solution::Solution(Plan plan) : plan_(std::move(plan)) {}

const Plan& Solution::plan() const noexcept { return plan_; }

Plan& Solution::plan() noexcept { return plan_; }

void Solution::set_routes(DayRoutes routes) {
    const Day day = routes.day();
    if (!plan_.contains(day)) {
        throw Error(Status::invalid_argument, "day " + std::to_string(day) + " is outside the plan");
    }
    std::erase_if(routes_, [&](const auto& entry) { return !plan_.contains(entry.first); });
    routes_.insert_or_assign(day, std::move(routes));
}

const DayRoutes* Solution::routes(Day day) const noexcept {
    if (!plan_.contains(day)) {
        return nullptr;
    }
    auto it = routes_.find(day);
    return it == routes_.end() ? nullptr : &it->second;
}

void Solution::clear_routes(Day day) { routes_.erase(day); }

}
