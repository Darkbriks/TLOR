/**
 * @file model.h
 * @brief Instances, fleets, states, parameters, plans, routes and solutions of the C interface.
 *
 * Every _create copies its inputs and returns TLOR_ERROR_INVALID_ARGUMENT on a null output pointer. Every _destroy
 * accepts NULL. Pointers returned by accessors stay valid until the handle is modified or destroyed. Matrices are
 * row-major ([from * node_count + to]) and demands day-major ([day * node_count + node]).
 */
#ifndef TLOR_C_MODEL_H
#define TLOR_C_MODEL_H

#include <tlor/c/common.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tlor_instance tlor_instance;
typedef struct tlor_fleet tlor_fleet;
typedef struct tlor_state tlor_state;
typedef struct tlor_params tlor_params;
typedef struct tlor_plan tlor_plan;
typedef struct tlor_day_routes tlor_day_routes;
typedef struct tlor_solution tlor_solution;

/** @brief Arrays an instance is built from. */
typedef struct tlor_instance_desc {
    int32_t node_count;
    int32_t depot;
    int32_t day_count;
    const double *distance_m;
    const double *duration_s;
    const double *service_s;
    const double *capacity_kg;
    const uint8_t *active;
    const double *demand_kg;
} tlor_instance_desc;

/** @brief Builds an instance from arrays. */
TLOR_API tlor_status tlor_instance_create(const tlor_instance_desc *desc, tlor_instance **out);
/** @brief Frees an instance. */
TLOR_API void tlor_instance_destroy(tlor_instance *instance);
/** @brief Sets a named per-node attribute (node_count values, NaN meaning unset). */
TLOR_API tlor_status tlor_instance_set_attribute(tlor_instance *instance, const char *name, const double *values);
/** @brief Number of nodes, depot included. */
TLOR_API int32_t tlor_instance_node_count(const tlor_instance *instance);
/** @brief Index of the depot. */
TLOR_API int32_t tlor_instance_depot(const tlor_instance *instance);
/** @brief Number of days of the predicted demand. */
TLOR_API int32_t tlor_instance_day_count(const tlor_instance *instance);
/** @brief Distance matrix in metres. */
TLOR_API const double *tlor_instance_distance_m(const tlor_instance *instance);
/** @brief Duration matrix in seconds. */
TLOR_API const double *tlor_instance_duration_s(const tlor_instance *instance);
/** @brief Service time of every node in seconds. */
TLOR_API const double *tlor_instance_service_s(const tlor_instance *instance);
/** @brief Capacity of every node in kilograms. */
TLOR_API const double *tlor_instance_capacity_kg(const tlor_instance *instance);
/** @brief Active flag of every node. */
TLOR_API const uint8_t *tlor_instance_active(const tlor_instance *instance);
/** @brief Predicted demand in kilograms, day-major. */
TLOR_API const double *tlor_instance_demand_kg(const tlor_instance *instance);
/** @brief Values of a named attribute, or NULL when absent. */
TLOR_API const double *tlor_instance_attribute(const tlor_instance *instance, const char *name);

/** @brief Builds a fleet; available is day_count × truck_count, NULL meaning always available. */
TLOR_API tlor_status tlor_fleet_create(int32_t truck_count, const double *capacity_kg, const double *max_work_s,
                                       int32_t day_count, const uint8_t *available, tlor_fleet **out);
/** @brief Frees a fleet. */
TLOR_API void tlor_fleet_destroy(tlor_fleet *fleet);
/** @brief Number of trucks. */
TLOR_API int32_t tlor_fleet_truck_count(const tlor_fleet *fleet);
/** @brief Number of days. */
TLOR_API int32_t tlor_fleet_day_count(const tlor_fleet *fleet);
/** @brief Capacity of every truck in kilograms. */
TLOR_API const double *tlor_fleet_capacity_kg(const tlor_fleet *fleet);
/** @brief Maximum work time of every truck in seconds. */
TLOR_API const double *tlor_fleet_max_work_s(const tlor_fleet *fleet);
/** @brief Availability, day-major. */
TLOR_API const uint8_t *tlor_fleet_available(const tlor_fleet *fleet);

/** @brief Builds a state; NULL arrays mean empty containers never collected. */
TLOR_API tlor_status tlor_state_create(int32_t node_count, const double *level_kg, const int32_t *last_collection,
                                       tlor_state **out);
/** @brief Frees a state. */
TLOR_API void tlor_state_destroy(tlor_state *state);
/** @brief Number of nodes. */
TLOR_API int32_t tlor_state_node_count(const tlor_state *state);
/** @brief Waste in each container in kilograms. */
TLOR_API const double *tlor_state_level_kg(const tlor_state *state);
/** @brief Last collection day of each node, -1 when never. */
TLOR_API const int32_t *tlor_state_last_collection(const tlor_state *state);

/** @brief Empty set of parameters. */
TLOR_API tlor_status tlor_params_create(tlor_params **out);
/** @brief Frees parameters. */
TLOR_API void tlor_params_destroy(tlor_params *params);
/** @brief Sets an integer value. */
TLOR_API tlor_status tlor_params_set_int(tlor_params *params, const char *name, int64_t value);
/** @brief Sets a real value. */
TLOR_API tlor_status tlor_params_set_double(tlor_params *params, const char *name, double value);
/** @brief Sets a boolean value (0 or not 0). */
TLOR_API tlor_status tlor_params_set_bool(tlor_params *params, const char *name, int value);
/** @brief Sets a string value. */
TLOR_API tlor_status tlor_params_set_string(tlor_params *params, const char *name, const char *value);
/** @brief Number of values. */
TLOR_API size_t tlor_params_size(const tlor_params *params);
/** @brief Name of the value at index, names being sorted; NULL out of range. */
TLOR_API const char *tlor_params_key(const tlor_params *params, size_t index);
/** @brief Type of a value; TLOR_ERROR_INVALID_PARAM when absent. */
TLOR_API tlor_status tlor_params_type(const tlor_params *params, const char *name, tlor_param_type *out);
/** @brief Integer value; TLOR_ERROR_INVALID_PARAM when absent or of another type. */
TLOR_API tlor_status tlor_params_get_int(const tlor_params *params, const char *name, int64_t *out);
/** @brief Real value, an integer being converted; TLOR_ERROR_INVALID_PARAM otherwise. */
TLOR_API tlor_status tlor_params_get_double(const tlor_params *params, const char *name, double *out);
/** @brief Boolean value as 0 or 1; TLOR_ERROR_INVALID_PARAM when absent or of another type. */
TLOR_API tlor_status tlor_params_get_bool(const tlor_params *params, const char *name, int *out);
/** @brief String value; TLOR_ERROR_INVALID_PARAM when absent or of another type. */
TLOR_API tlor_status tlor_params_get_string(const tlor_params *params, const char *name, const char **out);

/** @brief Empty plan of horizon days from start_day. */
TLOR_API tlor_status tlor_plan_create(int32_t start_day, int32_t horizon, tlor_plan **out);
/** @brief Frees a plan. */
TLOR_API void tlor_plan_destroy(tlor_plan *plan);
/** @brief Collects a point on a day. */
TLOR_API tlor_status tlor_plan_add(tlor_plan *plan, int32_t day, int32_t point);
/** @brief Stops collecting a point on a day. */
TLOR_API tlor_status tlor_plan_remove(tlor_plan *plan, int32_t day, int32_t point);
/** @brief First day. */
TLOR_API int32_t tlor_plan_start(const tlor_plan *plan);
/** @brief Number of days. */
TLOR_API int32_t tlor_plan_horizon(const tlor_plan *plan);
/** @brief Points collected on a day, sorted. */
TLOR_API tlor_status tlor_plan_points(const tlor_plan *plan, int32_t day, const int32_t **points, size_t *count);
/** @brief Replaces dst by a copy of src. */
TLOR_API tlor_status tlor_plan_assign(tlor_plan *dst, const tlor_plan *src);

/** @brief No route on a day. */
TLOR_API tlor_status tlor_day_routes_create(int32_t day, tlor_day_routes **out);
/** @brief Frees routes. */
TLOR_API void tlor_day_routes_destroy(tlor_day_routes *routes);
/** @brief Appends a route of count nodes driven by a truck. */
TLOR_API tlor_status tlor_day_routes_add(tlor_day_routes *routes, int32_t truck, const int32_t *nodes, size_t count);
/** @brief Day of the routes. */
TLOR_API int32_t tlor_day_routes_day(const tlor_day_routes *routes);
/** @brief Number of routes. */
TLOR_API size_t tlor_day_routes_count(const tlor_day_routes *routes);
/** @brief Truck and nodes of the route at index. */
TLOR_API tlor_status tlor_day_routes_get(const tlor_day_routes *routes, size_t index, int32_t *truck,
                                         const int32_t **nodes, size_t *count);
/** @brief Replaces dst by a copy of src. */
TLOR_API tlor_status tlor_day_routes_assign(tlor_day_routes *dst, const tlor_day_routes *src);

/** @brief Solution of a copy of a plan, with no routes. */
TLOR_API tlor_status tlor_solution_create(const tlor_plan *plan, tlor_solution **out);
/** @brief Frees a solution. */
TLOR_API void tlor_solution_destroy(tlor_solution *solution);
/** @brief Plan of the solution, borrowed. */
TLOR_API const tlor_plan *tlor_solution_plan(const tlor_solution *solution);
/** @brief Sets a copy of routes for their day. */
TLOR_API tlor_status tlor_solution_set_routes(tlor_solution *solution, const tlor_day_routes *routes);
/** @brief Routes of a day, borrowed until the solution changes; NULL when the day is not routed. */
TLOR_API const tlor_day_routes *tlor_solution_routes(const tlor_solution *solution, int32_t day);
/** @brief Replaces dst by a copy of src. */
TLOR_API tlor_status tlor_solution_assign(tlor_solution *dst, const tlor_solution *src);

#ifdef __cplusplus
}
#endif

#endif
