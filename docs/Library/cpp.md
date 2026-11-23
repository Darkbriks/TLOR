---
title: C++
description: Use the library from a C++ program, add a built-in method that every language can call, and test it.
sidebar:
  order: 6
---

C++ is the fastest way to use the library, and the only way to add a method
that ships with it: a method added in C++ is available by name from Python,
from C and from the `TLOR` program. The C++ interface lives in
`include/tlor/*.hpp`; every declaration is documented in the
[API reference](/TLOR/api/).

## 1. The C++ interface

| Header | Holds |
| --- | --- |
| `tlor/instance.hpp`, `fleet.hpp`, `state.hpp`, `demand.hpp` | the model: `Instance` (built from `InstanceData`), `Fleet` of `Truck`s, `State`, `Demand` |
| `tlor/plan.hpp`, `routes.hpp`, `solution.hpp` | `Plan`, `Route`, `DayRoutes`, `Solution` |
| `tlor/params.hpp` | `Params`, a map from names to `std::int64_t`, `double`, `bool` or `std::string` |
| `tlor/steps.hpp` | `schedule`, `route`, `improve` |
| `tlor/simulate.hpp`, `evaluate.hpp`, `check.hpp` | `simulate` and `day_loads`, `evaluate` and `objective`, `check` |
| `tlor/pipeline.hpp` | `RollingOptions`, `run_rolling` |
| `tlor/registry.hpp` | `Registry`, `MethodInfo`, `ParamSpec`, the request structs |
| `tlor/report.hpp` | `Report`, the `Phase` guard and `count` |
| `tlor/random.hpp` | `Random`, the platform-independent generator |
| `tlor/io.hpp` | `load_instance`, `write_solution`, `parse_date`, `format_date` |

Everything is in the namespace `tlor`. The functions take the same arguments
as their [Python counterparts](../python/#4-the-steps), in the same order; a
null `Report*` disables timing.

## 2. A C++ program

The static library `tlor_core` holds everything. A program of your own needs
two lines in the root `CMakeLists.txt`:

```cmake title="CMakeLists.txt"
add_executable(rolling_baseline experiments/rolling_baseline.cpp)
target_link_libraries(rolling_baseline PRIVATE tlor_core)
```

```cpp title="experiments/rolling_baseline.cpp"
#include <tlor/error.hpp>
#include <tlor/fleet.hpp>
#include <tlor/io.hpp>
#include <tlor/pipeline.hpp>
#include <tlor/report.hpp>
#include <tlor/state.hpp>

#include <iostream>
#include <vector>

int main() {
    try {
        const auto loaded = tlor::load_instance({"data/tables", "Maps/offline map/data", {}, true});
        const tlor::Instance& inst = loaded.instance;
        const tlor::Fleet fleet(std::vector<tlor::Truck>(6, tlor::Truck{8000, 8 * 3600}), inst.day_count(), {});

        tlor::RollingOptions options;
        options.first_day = 0;
        options.last_day = inst.day_count() - 1;
        options.horizon = 7;
        options.step = 1;
        options.schedule = {"threshold", {}};
        options.route = {"nearest_neighbour", {}};
        options.weights.set("distance_m", 1.0);
        options.weights.set("overflow_point_days", 1e6);

        tlor::Report report;
        const auto initial = tlor::State::initial(inst.node_count());
        const auto result = tlor::run_rolling(inst, fleet, initial, options, nullptr, &report);

        std::cout << result.evaluation.total.at("distance_m") << " m, " << result.violations.size()
                  << " violations\n";
        tlor::write_solution("local/runs/cpp", inst, initial, result.solution, inst.demand(), loaded.first_date);
        report.write_json("local/runs/cpp/report.json");
    } catch (const tlor::Error& e) {
        std::cerr << tlor::to_string(e.status()) << ": " << e.what() << '\n';
        return 1;
    }
}
```

```bash frame="terminal"
cmake --build --preset linux-release
./build/linux-release/bin/rolling_baseline
```

An empty availability vector in `Fleet` means every truck works every day;
otherwise it holds `day_count × truck_count` flags, day by day.

## 3. Adding a built-in method

A built-in method is a function of the request of its step, plus a function
describing it, both in `src/methods/<step>/`. Here, a route method that
visits the points in index order (the same as the
[Python example](../python/#81-a-route-method)).

### 3.1 The method

```cpp title="src/methods/route/in_order.cpp"
#include "methods/methods.hpp"

#include <tlor/report.hpp>

namespace tlor {

DayRoutes route_in_order(const RouteRequest& request) {
    const Node depot = request.instance.depot();
    const auto limit = request.params.get_int("max_points");
    std::vector<TruckId> trucks = request.fleet.available_trucks(request.day);
    if (trucks.empty()) {
        trucks.push_back(0);
    }
    Phase phase(request.report, "build");
    DayRoutes routes(request.day);
    Route route{trucks[0], {depot}};
    double load = 0;
    std::size_t index = 0;
    for (std::size_t i = 0; i < request.points.size(); ++i) {
        const double capacity = request.fleet.truck(trucks[index]).capacity_kg;
        const auto stops = static_cast<std::int64_t>(route.nodes.size()) - 1;
        if (stops > 0 && (load + request.loads_kg[i] > capacity || stops == limit)) {
            route.nodes.push_back(depot);
            routes.add(std::move(route));
            index = (index + 1) % trucks.size();
            route = Route{trucks[index], {depot}};
            load = 0;
        }
        route.nodes.push_back(request.points[i]);
        load += request.loads_kg[i];
    }
    if (route.nodes.size() > 1) {
        route.nodes.push_back(depot);
        routes.add(std::move(route));
    }
    count(request.report, "routes", static_cast<std::int64_t>(routes.size()));
    return routes;
}

MethodInfo in_order_info() {
    return MethodInfo{
        "in_order",
        "Visits the points in index order, one truck after the other",
        {
            {"max_points", ParamType::integer, std::int64_t{20}, 1.0, std::nullopt, "Most points on one route"},
        },
        true,
    };
}

}
```

### 3.2 Registering it

Declare both functions in `src/methods/methods.hpp`, next to the others:

```cpp title="src/methods/methods.hpp"
DayRoutes route_in_order(const RouteRequest& request);
MethodInfo in_order_info();
```

and add one line to `register_builtin_methods` in `src/methods/builtin.cpp`:

```cpp title="src/methods/builtin.cpp"
registry.add_route(in_order_info(), route_in_order);
```

The file is found by CMake on its own. After a rebuild, `in_order` is
available everywhere: `tlor.route(..., "in_order")` in Python, `--route
in_order` on the command line.

### 3.3 What the request gives

| Request | Fields |
| --- | --- |
| `ScheduleRequest` | `instance`, `fleet`, `start_state`, `start_day`, `horizon`, `params`, `weights`, `report` |
| `RouteRequest` | `instance`, `fleet`, `day`, `points`, `loads_kg`, `params`, `weights`, `report` |
| `ImproveRequest` | `instance`, `fleet`, `start_state`, `solution`, `route_method`, `route_params`, `params`, `weights`, `report` |

- `params` is already checked and complete: `get_int`, `get_double`,
  `get_bool` and `get_string` never miss a declared parameter.
- `report` may be null. `Phase phase(request.report, "name");` times the
  enclosing block and does nothing when the report is null; so does
  `count(request.report, "name", n)`. Use phases for a few coarse sections,
  counters inside loops.
- An improve method re-routes a day with
  `route(request.instance, request.fleet, day, points, loads, request.route_method, request.route_params, request.weights, request.report)`,
  and gets the loads from `simulate` and `day_loads`, as `shift_day` does in
  `src/methods/improve/shift_day.cpp`.
- Randomness comes from `tlor::Random rng(seed)`, with `rng.below(n)` and
  `rng.shuffle(values)`, never from `<random>` distributions, whose results
  differ between standard libraries. Declare a `seed` parameter.

### 3.4 Rules

- Return a plan of exactly `start_day` and `horizon`, routes of `day`, or a
  solution over the same days; anything else is an `internal_error`.
- Return a best-effort result when the constraints cannot all be met;
  throw only when nothing can be built, with
  `throw tlor::Error(tlor::Status::infeasible, "why")`. A bad parameter value
  that the bounds cannot express (such as an unknown string) is
  `Status::invalid_param`.
- `builtin = true` in the `MethodInfo` keeps the method from being
  unregistered. The parameter bounds are inclusive.
- No project constant in the method: take the depot, the sizes and the days
  from the request.

### 3.5 A method only for your program

A program can also register a method at run time, without touching the
library, with `Registry::global().add_route(info, fn)` (and `add_schedule`,
`add_improve`). `fn` may be a lambda; leave `builtin` false. The method then
exists only in that program.

## 4. Errors

Every failure throws `tlor::Error`, a `std::runtime_error` with a
`status()` of type `tlor::Status` (`invalid_argument`, `unknown_method`,
`invalid_param`, `io_error`, `infeasible`, `callback_error`,
`internal_error`); `tlor::to_string(status)` gives its name. The steps check
their inputs before doing any work.

## 5. Testing

Tests use [doctest](https://github.com/doctest/doctest) and live in
`tests/core/`, one file per module, found by CMake on its own. Each
`TEST_CASE` becomes a CTest test; name it `<module>: <behaviour>`. The
fixture in `tests/support/synthetic.hpp` gives a 6-node instance
(`small_instance()`, depot 5, point 4 inactive, 7 days) and a fleet
(`small_fleet(capacity, max_work, trucks)`, no truck on day 5) whose values
are listed in `tests/support/small_arrays.hpp`.

```cpp title="tests/core/test_in_order.cpp"
#include <doctest/doctest.h>

#include <tlor/steps.hpp>

#include "support/synthetic.hpp"

TEST_CASE("in_order: a route closes when the next point does not fit") {
    auto inst = tlor::test::small_instance();
    const std::vector<tlor::Node> points{0, 1, 2, 3};
    const std::vector<double> loads{60, 20, 40, 100};
    tlor::Params none;
    auto routes = tlor::route(inst, tlor::test::small_fleet(), 1, points, loads, "in_order", none, none, nullptr);
    REQUIRE(routes.size() == 2);
    CHECK(routes.routes()[0].nodes == std::vector<tlor::Node>{5, 0, 1, 2, 5});
    CHECK(routes.routes()[1].nodes == std::vector<tlor::Node>{5, 3, 5});
    CHECK(routes.routes()[1].truck == 1);
}
```

```bash frame="terminal"
cmake --build --preset linux-debug
ctest --preset linux-debug -R '^in_order:'
```
