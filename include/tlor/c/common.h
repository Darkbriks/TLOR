/**
 * @file common.h
 * @brief Status codes, error messages and version of the C interface.
 *
 * Every function returning a tlor_status clears the last error on entry and sets it on failure. Strings, file
 * paths included, are UTF-8.
 */
#ifndef TLOR_C_COMMON_H
#define TLOR_C_COMMON_H

#include <stddef.h>
#include <stdint.h>

#include <tlor/c/export.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Outcome of a call. */
typedef enum tlor_status {
    TLOR_OK = 0,
    TLOR_ERROR_INVALID_ARGUMENT = 1,
    TLOR_ERROR_UNKNOWN_METHOD = 2,
    TLOR_ERROR_INVALID_PARAM = 3,
    TLOR_ERROR_IO = 4,
    TLOR_ERROR_INFEASIBLE = 5,
    TLOR_ERROR_CALLBACK = 6,
    TLOR_ERROR_INTERNAL = 7
} tlor_status;

/** @brief Type of a parameter value. */
typedef enum tlor_param_type {
    TLOR_PARAM_INT = 0,
    TLOR_PARAM_DOUBLE = 1,
    TLOR_PARAM_BOOL = 2,
    TLOR_PARAM_STRING = 3
} tlor_param_type;

/** @brief Version of the library, such as "0.1.0". */
TLOR_API const char *tlor_version(void);

/** @brief Name of a status, such as "invalid_param", or "unknown". */
TLOR_API const char *tlor_status_name(tlor_status status);

/** @brief Message of the last failure on this thread; "" after a successful call. */
TLOR_API const char *tlor_last_error(void);

/** @brief Sets the message of the last failure; for callbacks reporting an error. */
TLOR_API void tlor_set_error(const char *message);

#ifdef __cplusplus
}
#endif

#endif
