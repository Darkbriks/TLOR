/**
 * @file indicators.hpp
 * @brief Names and values of the indicators an objective is built from.
 */
#pragma once

#include <array>
#include <map>
#include <string>
#include <string_view>

namespace tlor {

/** @brief Every indicator the library computes. */
inline constexpr std::array<std::string_view, 14> indicator_names{
    "distance_m",          "travel_s",         "service_s",
    "work_s",              "overtime_s",       "routes",
    "trucks_used",         "collections",      "collected_kg",
    "overflow_point_days", "overflow_kg_days", "low_fill_collections",
    "mean_fill_at_collection", "workload_imbalance_s",
};

/** @brief Whether a name is one of indicator_names. */
bool is_indicator_name(std::string_view name) noexcept;

/** @brief Indicator values by name. */
struct Indicators {
    /** @brief Values by name. */
    std::map<std::string, double, std::less<>> values;

    /** @brief Value of an indicator; throws Status::invalid_argument when absent. */
    [[nodiscard]] double at(std::string_view name) const;
};

}
