#include <tlor/routes.hpp>

namespace tlor {

DayRoutes::DayRoutes(Day day) : day_(day) {}

Day DayRoutes::day() const noexcept { return day_; }

void DayRoutes::add(Route route) { routes_.push_back(std::move(route)); }

std::span<const Route> DayRoutes::routes() const noexcept { return routes_; }

std::size_t DayRoutes::size() const noexcept { return routes_.size(); }

}
