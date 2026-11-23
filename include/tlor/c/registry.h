/**
 * @file registry.h
 * @brief Listing methods and registering external methods through callbacks.
 *
 * An external method receives a request whose accessors return borrowed handles, valid during the callback only.
 * The library creates its output before the call: an empty plan of the requested range, empty routes of the
 * requested day, or a copy of the solution to improve. A callback reports a failure by calling tlor_set_error and
 * returning any status other than TLOR_OK; the step then fails with TLOR_ERROR_CALLBACK and that message.
 */
#ifndef TLOR_C_REGISTRY_H
#define TLOR_C_REGISTRY_H

#include <tlor/c/common.h>
#include <tlor/c/model.h>
#include <tlor/c/report.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Step a method belongs to. */
typedef enum tlor_step { TLOR_STEP_SCHEDULE = 0, TLOR_STEP_ROUTE = 1, TLOR_STEP_IMPROVE = 2 } tlor_step;

/** @brief Declaration of one method parameter; only the default matching type is used. */
typedef struct tlor_param_spec {
    const char *name;
    tlor_param_type type;
    int64_t default_int;
    double default_double;
    int default_bool;
    const char *default_string;
    int has_min;
    double min;
    int has_max;
    double max;
    const char *description;
} tlor_param_spec;

typedef struct tlor_schedule_request tlor_schedule_request;
typedef struct tlor_route_request tlor_route_request;
typedef struct tlor_improve_request tlor_improve_request;

/** @brief Number of methods of a step. */
TLOR_API size_t tlor_method_count(tlor_step step);
/** @brief Name of the method at index, names being sorted; NULL out of range. */
TLOR_API const char *tlor_method_name(tlor_step step, size_t index);
/** @brief Description of a method; NULL when unknown. */
TLOR_API const char *tlor_method_description(tlor_step step, const char *name);
/** @brief Number of declared parameters of a method. */
TLOR_API tlor_status tlor_method_param_count(tlor_step step, const char *method, size_t *out);
/** @brief Declared parameter at index; strings are owned by the registry. */
TLOR_API tlor_status tlor_method_param(tlor_step step, const char *method, size_t index, tlor_param_spec *out);

/** @brief Instance of a schedule request. */
TLOR_API const tlor_instance *tlor_schedule_request_instance(const tlor_schedule_request *request);
/** @brief Fleet of a schedule request. */
TLOR_API const tlor_fleet *tlor_schedule_request_fleet(const tlor_schedule_request *request);
/** @brief Start state of a schedule request. */
TLOR_API const tlor_state *tlor_schedule_request_state(const tlor_schedule_request *request);
/** @brief First day to plan. */
TLOR_API int32_t tlor_schedule_request_start_day(const tlor_schedule_request *request);
/** @brief Number of days to plan. */
TLOR_API int32_t tlor_schedule_request_horizon(const tlor_schedule_request *request);
/** @brief Validated parameters of a schedule request. */
TLOR_API const tlor_params *tlor_schedule_request_params(const tlor_schedule_request *request);
/** @brief Objective weights of a schedule request. */
TLOR_API const tlor_params *tlor_schedule_request_weights(const tlor_schedule_request *request);
/** @brief Report of a schedule request, or NULL. */
TLOR_API tlor_report *tlor_schedule_request_report(const tlor_schedule_request *request);

/** @brief Instance of a route request. */
TLOR_API const tlor_instance *tlor_route_request_instance(const tlor_route_request *request);
/** @brief Fleet of a route request. */
TLOR_API const tlor_fleet *tlor_route_request_fleet(const tlor_route_request *request);
/** @brief Day to route. */
TLOR_API int32_t tlor_route_request_day(const tlor_route_request *request);
/** @brief Points to route; returns their number. */
TLOR_API size_t tlor_route_request_points(const tlor_route_request *request, const int32_t **points);
/** @brief Mass to collect at each point, in kilograms. */
TLOR_API const double *tlor_route_request_loads_kg(const tlor_route_request *request);
/** @brief Validated parameters of a route request. */
TLOR_API const tlor_params *tlor_route_request_params(const tlor_route_request *request);
/** @brief Objective weights of a route request. */
TLOR_API const tlor_params *tlor_route_request_weights(const tlor_route_request *request);
/** @brief Report of a route request, or NULL. */
TLOR_API tlor_report *tlor_route_request_report(const tlor_route_request *request);

/** @brief Instance of an improve request. */
TLOR_API const tlor_instance *tlor_improve_request_instance(const tlor_improve_request *request);
/** @brief Fleet of an improve request. */
TLOR_API const tlor_fleet *tlor_improve_request_fleet(const tlor_improve_request *request);
/** @brief Start state of an improve request. */
TLOR_API const tlor_state *tlor_improve_request_state(const tlor_improve_request *request);
/** @brief Solution to improve. */
TLOR_API const tlor_solution *tlor_improve_request_solution(const tlor_improve_request *request);
/** @brief Route method to re-route with. */
TLOR_API const char *tlor_improve_request_route_method(const tlor_improve_request *request);
/** @brief Parameters of the route method. */
TLOR_API const tlor_params *tlor_improve_request_route_params(const tlor_improve_request *request);
/** @brief Validated parameters of an improve request. */
TLOR_API const tlor_params *tlor_improve_request_params(const tlor_improve_request *request);
/** @brief Objective weights of an improve request. */
TLOR_API const tlor_params *tlor_improve_request_weights(const tlor_improve_request *request);
/** @brief Report of an improve request, or NULL. */
TLOR_API tlor_report *tlor_improve_request_report(const tlor_improve_request *request);

/** @brief External schedule method: fills out, an empty plan of the requested range. */
typedef tlor_status (*tlor_schedule_fn)(const tlor_schedule_request *request, tlor_plan *out, void *user_data);
/** @brief External route method: fills out, empty routes of the requested day. */
typedef tlor_status (*tlor_route_fn)(const tlor_route_request *request, tlor_day_routes *out, void *user_data);
/** @brief External improve method: modifies out, a copy of the solution to improve. */
typedef tlor_status (*tlor_improve_fn)(const tlor_improve_request *request, tlor_solution *out, void *user_data);

/** @brief Registers an external schedule method. */
TLOR_API tlor_status tlor_register_schedule_method(const char *name, const char *description,
                                                   const tlor_param_spec *specs, size_t spec_count,
                                                   tlor_schedule_fn fn, void *user_data);
/** @brief Registers an external route method. */
TLOR_API tlor_status tlor_register_route_method(const char *name, const char *description,
                                                const tlor_param_spec *specs, size_t spec_count, tlor_route_fn fn,
                                                void *user_data);
/** @brief Registers an external improve method. */
TLOR_API tlor_status tlor_register_improve_method(const char *name, const char *description,
                                                  const tlor_param_spec *specs, size_t spec_count,
                                                  tlor_improve_fn fn, void *user_data);
/** @brief Removes an external method; built-in methods cannot be removed. */
TLOR_API tlor_status tlor_unregister_method(tlor_step step, const char *name);

#ifdef __cplusplus
}
#endif

#endif
