/**
 * @file demand.hpp
 * @brief Waste generated at each node on each day.
 */
#pragma once

#include <tlor/types.hpp>

#include <cstdint>
#include <span>
#include <vector>

namespace tlor {

/** @brief Daily waste in kilograms, stored as kg[day * node_count + node]. */
class Demand {
public:
    /** @brief Builds a demand; throws Status::invalid_argument on a wrong size or a negative or non-finite value. */
    Demand(std::int32_t day_count, std::int32_t node_count, std::vector<double> kg);

    /** @brief Waste generated at a node on a day, in kilograms. */
    [[nodiscard]] double at(Day day, Node node) const noexcept;

    /** @brief Number of days. */
    [[nodiscard]] std::int32_t day_count() const noexcept;

    /** @brief Number of nodes. */
    [[nodiscard]] std::int32_t node_count() const noexcept;

    /** @brief All values, day-major. */
    [[nodiscard]] std::span<const double> data() const noexcept;

private:
    std::int32_t day_count_;
    std::int32_t node_count_;
    std::vector<double> kg_;
};

}
