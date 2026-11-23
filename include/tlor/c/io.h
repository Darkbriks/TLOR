/**
 * @file io.h
 * @brief Loading the project data and writing solutions through the C interface.
 *
 * Paths are UTF-8 and dates are "YYYY-MM-DD". Missing or unwritable files give TLOR_ERROR_IO, malformed data
 * TLOR_ERROR_INVALID_ARGUMENT.
 */
#ifndef TLOR_C_IO_H
#define TLOR_C_IO_H

#include <tlor/c/common.h>
#include <tlor/c/model.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Loads an instance from the project files.
 *
 * A NULL service_time_csv means exemple_points_service_time.csv in map_data_dir. When first_date is not NULL it
 * receives the date of day 0 and a terminating NUL. *out is NULL on failure.
 */
TLOR_API tlor_status tlor_instance_load(const char *tables_dir, const char *map_data_dir,
                                        const char *service_time_csv, int load_point_policy, tlor_instance **out,
                                        char first_date[11]);
/**
 * @brief Writes the routes files, collection_schedule.csv and route_statistics.csv into out_dir.
 *
 * A NULL start means the initial state at the plan's first day; a NULL demand_kg means the predicted demand,
 * otherwise day_count × node_count values, day-major. first_date is the date of day 0.
 */
TLOR_API tlor_status tlor_write_solution(const char *out_dir, const tlor_instance *instance, const tlor_state *start,
                                         const tlor_solution *solution, const double *demand_kg,
                                         const char *first_date);
/** @brief Writes one day's routes as a JSON list of node lists. */
TLOR_API tlor_status tlor_write_routes_json(const char *path, const tlor_day_routes *routes);

#ifdef __cplusplus
}
#endif

#endif
