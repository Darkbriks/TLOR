/**
 * @file
 * @brief Loading the project data into an instance and writing solutions to files.
 */
#pragma once

#include <tlor/demand.hpp>
#include <tlor/instance.hpp>
#include <tlor/routes.hpp>
#include <tlor/solution.hpp>
#include <tlor/state.hpp>

#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>

namespace tlor {

/** @brief Parses a "YYYY-MM-DD" date; throws invalid_argument when the text is not a valid date. */
std::chrono::sys_days parse_date(std::string_view text);

/** @brief Formats a date as "YYYY-MM-DD". */
std::string format_date(std::chrono::sys_days date);

/** @brief Where load_instance finds the data files. */
struct LoadOptions {
    /** @brief Directory holding active_points.csv, daily_demand.csv and point_policy.csv. */
    std::filesystem::path tables_dir;
    /** @brief Directory holding distances.json and durations.json. */
    std::filesystem::path map_data_dir;
    /** @brief Service-time CSV; empty means map_data_dir / "exemple_points_service_time.csv". */
    std::filesystem::path service_time_csv;
    /** @brief Loads point_policy.csv into the "threshold" attribute (NaN for points without a policy). */
    bool load_point_policy = true;
};

/** @brief A loaded instance and the date of its day 0. */
struct LoadedInstance {
    /** @brief The instance; its demand is the predicted daily demand. */
    Instance instance;
    /** @brief Date of day 0, the earliest date of the demand table. */
    std::chrono::sys_days first_date;
};

/** @brief Loads an instance from the project files; missing files are io_error, malformed ones invalid_argument. */
LoadedInstance load_instance(const LoadOptions& options);

/** @brief Writes one day's routes as a JSON list of node lists, each starting and ending at the depot. */
void write_routes_json(const std::filesystem::path& file, const DayRoutes& routes);

/**
 * @brief Writes routes_<date>.json for every routed day, collection_schedule.csv and route_statistics.csv.
 *
 * Collected masses come from simulating the plan on @p demand from @p start, the state at the plan's first day.
 * The directory is created when missing, and routes_<date>.json files left in it by an earlier run are removed;
 * failures are io_error.
 */
void write_solution(const std::filesystem::path& out_dir, const Instance& instance, const State& start,
                    const Solution& solution, const Demand& demand, std::chrono::sys_days first_date);

}
