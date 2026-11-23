/**
 * @file operations.h
 * @brief Steps, simulation, evaluation, checker and rolling driver of the C interface.
 *
 * NULL params, weights and evaluation params mean empty; a NULL state means empty containers never collected; a
 * NULL demand_kg means the predicted demand, otherwise day_count × node_count values, day-major. Methods are
 * chosen by name.
 */
#ifndef TLOR_C_OPERATIONS_H
#define TLOR_C_OPERATIONS_H

#include <tlor/c/common.h>
#include <tlor/c/model.h>
#include <tlor/c/report.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tlor_indicators tlor_indicators;
typedef struct tlor_evaluation tlor_evaluation;
typedef struct tlor_violations tlor_violations;
typedef struct tlor_rolling_result tlor_rolling_result;

/** @brief Plans start_day to start_day + horizon - 1 with a schedule method. */
TLOR_API tlor_status tlor_schedule(const tlor_instance *instance, const tlor_fleet *fleet, const tlor_state *state,
                                   int32_t start_day, int32_t horizon, const char *method, const tlor_params *params,
                                   const tlor_params *weights, tlor_report *report, tlor_plan **out);
/** @brief Routes count points of one day with a route method; loads_kg[i] is the mass at points[i]. */
TLOR_API tlor_status tlor_route(const tlor_instance *instance, const tlor_fleet *fleet, int32_t day,
                                const int32_t *points, const double *loads_kg, size_t count, const char *method,
                                const tlor_params *params, const tlor_params *weights, tlor_report *report,
                                tlor_day_routes **out);
/** @brief Improves a solution with an improve method, re-routing with route_method. */
TLOR_API tlor_status tlor_improve(const tlor_instance *instance, const tlor_fleet *fleet, const tlor_state *state,
                                  const tlor_solution *solution, const char *route_method,
                                  const tlor_params *route_params, const char *method, const tlor_params *params,
                                  const tlor_params *weights, tlor_report *report, tlor_solution **out);
/** @brief Simulates days days from start_day; end_state and level_kg (days × node_count) may be NULL. */
TLOR_API tlor_status tlor_simulate(const tlor_instance *instance, const tlor_state *state, int32_t start_day,
                                   int32_t days, const tlor_plan *plan, const double *demand_kg, tlor_state **end_state,
                                   double *level_kg);
/** @brief Indicators of a solution, per day and in total. */
TLOR_API tlor_status tlor_evaluate(const tlor_instance *instance, const tlor_fleet *fleet, const tlor_state *state,
                                   const tlor_solution *solution, const double *demand_kg,
                                   const tlor_params *evaluation_params, tlor_evaluation **out);
/** @brief Frees an evaluation. */
TLOR_API void tlor_evaluation_destroy(tlor_evaluation *evaluation);
/** @brief Indicators over the whole plan, borrowed. */
TLOR_API const tlor_indicators *tlor_evaluation_total(const tlor_evaluation *evaluation);
/** @brief First evaluated day. */
TLOR_API int32_t tlor_evaluation_start_day(const tlor_evaluation *evaluation);
/** @brief Number of evaluated days. */
TLOR_API int32_t tlor_evaluation_day_count(const tlor_evaluation *evaluation);
/** @brief Indicators of the day start_day + offset, borrowed; NULL out of range. */
TLOR_API const tlor_indicators *tlor_evaluation_day(const tlor_evaluation *evaluation, int32_t offset);
/** @brief Number of indicator names. */
TLOR_API size_t tlor_indicator_count(void);
/** @brief Indicator name at index; NULL out of range. */
TLOR_API const char *tlor_indicator_name(size_t index);
/** @brief Value of an indicator; TLOR_ERROR_INVALID_ARGUMENT when absent. */
TLOR_API tlor_status tlor_indicators_get(const tlor_indicators *indicators, const char *name, double *out);
/** @brief Weighted sum of indicators. */
TLOR_API tlor_status tlor_objective(const tlor_indicators *indicators, const tlor_params *weights, double *out);

/** @brief One violation; message is borrowed from its list. */
typedef struct tlor_violation {
    int32_t kind;
    int32_t day;
    int32_t route;
    int32_t point;
    const char *message;
} tlor_violation;

/** @brief Every violation of a solution. */
TLOR_API tlor_status tlor_check(const tlor_instance *instance, const tlor_fleet *fleet, const tlor_state *state,
                                const tlor_solution *solution, const double *demand_kg, tlor_violations **out);
/** @brief Frees a list of violations. */
TLOR_API void tlor_violations_destroy(tlor_violations *violations);
/** @brief Number of violations. */
TLOR_API size_t tlor_violations_count(const tlor_violations *violations);
/** @brief Violation at index. */
TLOR_API tlor_status tlor_violations_get(const tlor_violations *violations, size_t index, tlor_violation *out);
/** @brief Name of a violation kind, such as "overflow", or "unknown". */
TLOR_API const char *tlor_violation_kind_name(int32_t kind);

/** @brief Settings of a rolling-horizon run; improve_method NULL means no improvement. */
typedef struct tlor_rolling_options {
    int32_t first_day;
    int32_t last_day;
    int32_t horizon;
    int32_t step;
    const char *schedule_method;
    const tlor_params *schedule_params;
    const char *route_method;
    const tlor_params *route_params;
    const char *improve_method;
    const tlor_params *improve_params;
    const tlor_params *weights;
    const tlor_params *evaluation_params;
    int check_each_step;
} tlor_rolling_options;

/** @brief Runs the rolling-horizon driver; realized_demand_kg NULL means the predicted demand. */
TLOR_API tlor_status tlor_run_rolling(const tlor_instance *instance, const tlor_fleet *fleet, const tlor_state *initial,
                                      const tlor_rolling_options *options, const double *realized_demand_kg,
                                      tlor_report *report, tlor_rolling_result **out);
/** @brief Frees a rolling result. */
TLOR_API void tlor_rolling_result_destroy(tlor_rolling_result *result);
/** @brief Solution of the run, borrowed. */
TLOR_API const tlor_solution *tlor_rolling_result_solution(const tlor_rolling_result *result);
/** @brief Evaluation of the run, borrowed. */
TLOR_API const tlor_evaluation *tlor_rolling_result_evaluation(const tlor_rolling_result *result);
/** @brief Violations of the run, borrowed. */
TLOR_API const tlor_violations *tlor_rolling_result_violations(const tlor_rolling_result *result);
/** @brief State after the last day, borrowed. */
TLOR_API const tlor_state *tlor_rolling_result_end_state(const tlor_rolling_result *result);

#ifdef __cplusplus
}
#endif

#endif
