/**
 * @file routes.hpp
 * @brief Routes driven on one day.
 */
#pragma once

#include <tlor/types.hpp>

#include <cstddef>
#include <span>
#include <vector>

namespace tlor {

/** @brief One route: a truck and the nodes it visits, starting and ending at the depot. */
struct Route {
    /** @brief Truck driving the route. */
    TruckId truck = 0;
    /** @brief Visited nodes, such as {depot, 0, 2, depot}. */
    std::vector<Node> nodes;

    /** @brief Same truck and same nodes. */
    bool operator==(const Route&) const = default;
};

/** @brief Routes of one day, in the order they were added. */
class DayRoutes {
public:
    /** @brief No route on this day. */
    explicit DayRoutes(Day day);

    /** @brief Day of the routes. */
    [[nodiscard]] Day day() const noexcept;

    /** @brief Appends a route, without validation. */
    void add(Route route);

    /** @brief All routes. */
    [[nodiscard]] std::span<const Route> routes() const noexcept;

    /** @brief Number of routes. */
    [[nodiscard]] std::size_t size() const noexcept;

    /** @brief Same day and same routes. */
    bool operator==(const DayRoutes&) const = default;

private:
    Day day_;
    std::vector<Route> routes_;
};

}
