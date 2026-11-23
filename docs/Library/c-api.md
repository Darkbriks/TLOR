---
title: C interface
description: How to call the library from C or any language with a C foreign-function interface, handles and their lifetime, status codes, and methods written as C callbacks.
sidebar:
  order: 7
---

The shared library `tlor` exports a plain C11 interface, declared in
`include/tlor/tlor.h`. It is what the Python package uses, and what any
other language (Julia, R, Rust, Java through JNA…) can use too. Every
function is listed with its documentation in the
[API reference](/TLOR/api/headers/tlor-h/); this page gives the rules they share.

## 1. Building and linking

Building any preset produces the shared library next to `TLOR`:
`build/<preset>/bin/libtlor.so` on Linux, `libtlor.dylib` on macOS,
`build\windows\bin\<Debug or Release>\libtlor.dll` on Windows. Include one
header and link the library:

```bash frame="terminal"
cc -std=c11 -I include rolling.c -L build/linux-release/bin -ltlor \
   -Wl,-rpath,"$PWD/build/linux-release/bin" -o rolling
```

The library exports only the `tlor_*` functions, so it does not clash with
the program's own C++ runtime.

## 2. Status codes and errors

Nearly every function returns a `tlor_status`:

| Code | Value | Meaning |
| --- | --- | --- |
| `TLOR_OK` | 0 | Success. |
| `TLOR_ERROR_INVALID_ARGUMENT` | 1 | A malformed or inconsistent input, including a NULL pointer where one is required. |
| `TLOR_ERROR_UNKNOWN_METHOD` | 2 | No method of that name. |
| `TLOR_ERROR_INVALID_PARAM` | 3 | A parameter, weight or evaluation parameter is wrong. |
| `TLOR_ERROR_IO` | 4 | A file cannot be read or written. |
| `TLOR_ERROR_INFEASIBLE` | 5 | A method could not build any result. |
| `TLOR_ERROR_CALLBACK` | 6 | An external method (C callback, Python) failed. |
| `TLOR_ERROR_INTERNAL` | 7 | A bug, or a method that returned the wrong days. |

After a failure, `tlor_last_error()` returns the message of the last failing
call made **by this thread**; `tlor_status_name(status)` returns
`"invalid_argument"` and so on. No C++ exception ever crosses the interface.

## 3. Handles and lifetime

- Every object is an opaque handle: `tlor_instance`, `tlor_fleet`,
  `tlor_state`, `tlor_params`, `tlor_plan`, `tlor_day_routes`,
  `tlor_solution`, `tlor_report`, and the results `tlor_evaluation`,
  `tlor_violations`, `tlor_rolling_result`.
- A function that creates an object writes it to its last argument, a
  `tlor_x **out`; you free it with `tlor_x_destroy`, which accepts NULL.
  Set the pointer to NULL before the call: on failure no object is created.
- Creation **copies** its inputs: the arrays you pass can be freed right
  after.
- A pointer returned by an accessor (an array, a string, a borrowed handle
  such as `tlor_solution_plan` or `tlor_rolling_result_evaluation`) belongs to
  the object it came from and stays valid until that object is modified or
  destroyed.
- Optional inputs accept NULL: a NULL state means the initial state (empty
  containers, never collected), a NULL demand the predicted demand, NULL
  params, weights or evaluation params an empty set, a NULL report no timing.

## 4. Arrays and text

- Matrices are row-major, from the origin: `distance_m[from * node_count + to]`.
- Demands are day-major, `demand_kg[day * node_count + node]`; so is fleet
  availability, `available[day * truck_count + truck]`.
- Node and day indices are `int32_t`, masses, distances and times `double`,
  flags `uint8_t` (0 or 1).
- Strings, file paths included, are UTF-8. Dates are `"YYYY-MM-DD"`.

## 5. A complete program

Loads the data, runs the rolling driver, prints an indicator and writes the
routes:

```c title="rolling.c"
#include <tlor/tlor.h>

#include <stdio.h>

static int fail(const char *what) {
    fprintf(stderr, "%s failed: %s\n", what, tlor_last_error());
    return 1;
}

int main(void) {
    tlor_instance *inst = NULL;
    char first_date[11];
    if (tlor_instance_load("data/tables", "Maps/offline map/data", NULL, 1, &inst, first_date) != TLOR_OK) {
        return fail("loading");
    }
    double capacity[6], work[6];
    for (int t = 0; t < 6; ++t) {
        capacity[t] = 8000;
        work[t] = 8 * 3600;
    }
    tlor_fleet *fleet = NULL;
    if (tlor_fleet_create(6, capacity, work, tlor_instance_day_count(inst), NULL, &fleet) != TLOR_OK) {
        return fail("fleet");
    }
    tlor_params *weights = NULL;
    tlor_params_create(&weights);
    tlor_params_set_double(weights, "distance_m", 1.0);

    tlor_rolling_options options = {0};
    options.first_day = 0;
    options.last_day = tlor_instance_day_count(inst) - 1;
    options.horizon = 7;
    options.step = 1;
    options.schedule_method = "threshold";
    options.route_method = "nearest_neighbour";
    options.weights = weights;

    tlor_rolling_result *result = NULL;
    if (tlor_run_rolling(inst, fleet, NULL, &options, NULL, NULL, &result) != TLOR_OK) {
        return fail("rolling");
    }
    double distance = 0;
    tlor_indicators_get(tlor_evaluation_total(tlor_rolling_result_evaluation(result)), "distance_m", &distance);
    printf("from %s: %.0f m, %zu violations\n", first_date, distance,
           tlor_violations_count(tlor_rolling_result_violations(result)));
    if (tlor_write_solution("local/runs/c", inst, NULL, tlor_rolling_result_solution(result), NULL, first_date) !=
        TLOR_OK) {
        return fail("writing");
    }
    tlor_rolling_result_destroy(result);
    tlor_params_destroy(weights);
    tlor_fleet_destroy(fleet);
    tlor_instance_destroy(inst);
    return 0;
}
```

The other steps follow the same pattern: `tlor_schedule`, `tlor_route`,
`tlor_improve`, `tlor_simulate`, `tlor_evaluate`, `tlor_check`, with the
arguments of their [Python counterparts](../python/#4-the-steps).

## 6. Methods written in C

A method written in C (or in any language that can hand out a C function
pointer) is registered with a callback and an opaque `user_data` pointer:

```c
tlor_status tlor_register_route_method(const char *name, const char *description,
                                       const tlor_param_spec *specs, size_t spec_count,
                                       tlor_route_fn fn, void *user_data);
```

and likewise `tlor_register_schedule_method` and
`tlor_register_improve_method`; `tlor_unregister_method(step, name)` removes
it. The name, description and specs are copied.

The library creates the output **before** calling the method: an empty plan
of the requested days, empty routes of the requested day, or a copy of the
solution to improve. The method fills or modifies it, and reads its inputs
through the `tlor_<step>_request_*` accessors (instance, fleet, state, day,
points and loads, params already checked and completed with their defaults,
weights, report). The request and everything borrowed from it are valid only
during the call.

```c
static tlor_status one_route_per_point(const tlor_route_request *request, tlor_day_routes *out,
                                       void *user_data) {
    (void)user_data;
    const int32_t depot = tlor_instance_depot(tlor_route_request_instance(request));
    int64_t truck = 0;
    tlor_params_get_int(tlor_route_request_params(request), "truck", &truck);
    const int32_t *points = NULL;
    const size_t count = tlor_route_request_points(request, &points);
    for (size_t i = 0; i < count; ++i) {
        const int32_t nodes[3] = {depot, points[i], depot};
        if (tlor_day_routes_add(out, (int32_t)truck, nodes, 3) != TLOR_OK) {
            return TLOR_ERROR_INTERNAL;
        }
    }
    return TLOR_OK;
}

static int register_one_route_per_point(void) {
    tlor_param_spec truck = {0};
    truck.name = "truck";
    truck.type = TLOR_PARAM_INT;
    truck.default_int = 0;
    truck.has_min = 1;
    truck.min = 0;
    truck.description = "Truck that drives every route";
    return tlor_register_route_method("one_route_per_point", "Each point on its own route", &truck, 1,
                                      one_route_per_point, NULL) == TLOR_OK;
}
```

To fail, call `tlor_set_error("why")` and return any status other than
`TLOR_OK`: the step then fails with `TLOR_ERROR_CALLBACK` and that message.
A method should return a best-effort result rather than fail when the
constraints cannot all be met, as described in
[Concepts](../concepts/#5-plan-routes-solution).

## 7. Threads

The library is not thread-safe: the method registry is shared by the whole
process, and handles must not be used from two threads at once. Only the
error message of `tlor_last_error()` is kept per thread.
