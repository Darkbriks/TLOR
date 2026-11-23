/**
 * @file report.h
 * @brief Time reports of the C interface.
 *
 * Phase and counter functions accept a NULL report and then do nothing.
 */
#ifndef TLOR_C_REPORT_H
#define TLOR_C_REPORT_H

#include <tlor/c/common.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tlor_report tlor_report;

/** @brief Empty report; records the start time. */
TLOR_API tlor_status tlor_report_create(tlor_report **out);
/** @brief Frees a report. */
TLOR_API void tlor_report_destroy(tlor_report *report);
/** @brief Adds a free key and value to the report's context. */
TLOR_API tlor_status tlor_report_set_context(tlor_report *report, const char *key, const char *value);
/** @brief Opens a phase in the innermost container. */
TLOR_API tlor_status tlor_report_phase_begin(tlor_report *report, const char *name);
/** @brief Closes the innermost phase; TLOR_ERROR_INVALID_ARGUMENT when it is not a phase. */
TLOR_API tlor_status tlor_report_phase_end(tlor_report *report);
/** @brief Adds delta to a counter of the innermost container. */
TLOR_API tlor_status tlor_report_count(tlor_report *report, const char *name, int64_t delta);
/** @brief Writes the report as JSON; TLOR_ERROR_IO on failure. */
TLOR_API tlor_status tlor_report_write_json(const tlor_report *report, const char *path);

#ifdef __cplusplus
}
#endif

#endif
