---
title: Python package
description: Every class and function of the tlor Python package, with the shapes of the arrays it takes and returns, and how to write a schedule, route or improve method in Python.
sidebar:
  order: 5
---

The `tlor` package (in `python/tlor/`) drives the library from Python. It is
plain Python over `ctypes` and numpy: there is nothing to compile besides the
library itself, and every result is checked by the same C++ code as
everywhere else.

## 1. Setting up

Build the library with a preset, create the virtual environment and point
Python at `python/`, as in
[Build and run](../../bootstrap/build-and-run/#5-python-package). Then:

```python
import tlor
print(tlor.__version__)
```

The package loads the shared library from `TLOR_LIBRARY` when that variable
holds a path, and otherwise from the first `build/*/bin/` of the repository
that has one, **release builds first**. After changing C++ code, rebuild the
preset you load from: a stale release build is picked before a fresh debug
one. An `ImportError` at `import tlor` means no library was found.

## 2. Conventions

- **Arrays** go in as anything numpy accepts (lists, arrays) and come out as
  new numpy arrays: `float64` for masses, distances and times, `int32` for
  node and day indices, `bool` for flags. Changing a returned array never
  changes the library's object.
- **Shapes** follow the [Concepts](../concepts/): a matrix is
  `(node_count, node_count)` with the origin as the row, a demand is
  `(day_count, node_count)`, an availability table is
  `(day_count, truck_count)`.
- **Parameters and weights** are dicts: `{"threshold": 0.9}`,
  `{"distance_m": 1.0}`. Values may be `int`, `float`, `bool` or `str`.
- **Methods** are chosen by name, as a string, or as a `(name, params)` pair
  where a function takes both.
- A `state` argument may be `None` for the initial state (empty containers,
  never collected), and a `demand_kg` argument may be `None` for the
  instance's predicted demand.

## 3. The model

### 3.1 `Instance`

```python
inst = tlor.Instance.load(tables, maps, service_time=None, point_policy=True)
```

Loads the project data: `tables` is the directory of `active_points.csv`,
`daily_demand.csv` and `point_policy.csv` (usually `data/tables`), `maps` the
directory of `distances.json` and `durations.json` (usually
`Maps/offline map/data`). `service_time` is the service-time CSV, by default
`exemple_points_service_time.csv` in `maps`. With `point_policy=True` the
thresholds of `point_policy.csv` become the `threshold` attribute. Paths may
be `str` or `pathlib.Path`. The [file formats](../files-and-cli/#1-input-files)
are described with the command line.

```python
inst = tlor.Instance(distance_m, duration_s, service_s, capacity_kg, active, demand_kg, depot)
```

Builds an instance from arrays, for tests or generated data:

```python
import numpy as np
import tlor

x = np.array([1.0, 2.0, 3.0, 0.0])                     # node 3 is the depot
distance = np.abs(x[:, None] - x[None, :]) * 1000.0
inst = tlor.Instance(
    distance_m=distance,
    duration_s=distance / 10.0,
    service_s=[60, 60, 60, 300],
    capacity_kg=[100, 100, 100, 0],
    active=[True, True, True, False],
    demand_kg=np.tile([20.0, 35.0, 10.0, 0.0], (5, 1)),  # 5 days
    depot=3,
)
print(inst.node_count, inst.day_count, inst.first_date)  # 4 5 None
```

| Member | Value |
| --- | --- |
| `node_count`, `depot`, `day_count` | `int` |
| `first_date` | `datetime.date` of day 0 for a loaded instance, `None` for one built from arrays |
| `distance_m`, `duration_s` | `(n, n)` arrays |
| `service_s`, `capacity_kg` | `(n,)` arrays |
| `active` | `(n,)` bool array |
| `demand_kg` | `(day_count, n)` array of the predicted demand |
| `set_attribute(name, values)` | attaches an `(n,)` array; NaN means "not set" for a node |
| `attribute(name)` | the `(n,)` array, or `None` when absent |

The arrays are fixed once the instance exists; only attributes can be added
or replaced.

### 3.2 `Fleet`

```python
fleet = tlor.Fleet(capacity_kg, max_work_s, day_count, available=None)
```

One entry per truck in `capacity_kg` and `max_work_s`. `day_count` must be
the instance's. `available` is a `(day_count, truck_count)` bool array,
`None` meaning every truck works every day. Members: `truck_count`,
`day_count`, `capacity_kg`, `max_work_s`, `available`.

Weekends off, from the dates of a loaded instance:

```python
import datetime
import numpy as np

inst = tlor.Instance.load("data/tables", "Maps/offline map/data")
dates = [inst.first_date + datetime.timedelta(days=d) for d in range(inst.day_count)]
working = np.array([date.weekday() < 5 for date in dates])
fleet = tlor.Fleet([8000] * 6, [8 * 3600] * 6, inst.day_count, np.repeat(working[:, None], 6, axis=1))
```

### 3.3 `State`

```python
state = tlor.State(node_count, level_kg=None, last_collection=None)
state = tlor.State.initial(inst)
```

`level_kg` is the waste in each container in kg, `last_collection` the last
collection day of each node, `-1` for never; both default to the initial
values. Members: `node_count`, `level_kg`, `last_collection`.

### 3.4 `Plan`, `DayRoutes`, `Solution`

```python
plan = tlor.Plan(start, horizon)
plan.add(day, point)          # collect point on day
plan.remove(day, point)
plan.points(day)              # int32 array, sorted
plan.days()                   # {day: [points]} for the days with collections
plan.start, plan.horizon

routes = tlor.DayRoutes(day)
routes.add(truck, [depot, a, b, depot])
len(routes)
for truck, nodes in routes: ...
routes.day

solution = tlor.Solution(plan)        # copies the plan, no routes yet
solution.set_routes(routes)           # stores a copy, for routes.day
solution.routes(day)                  # a copy of that day's routes, or None
solution.plan                         # a copy of the plan
```

`solution.plan` returns a **copy**: to change the plan of a solution, change
the copy and build a new `Solution` from it, then set its routes again.

## 4. The steps

Each function below is described in [Steps](../steps/); here are the Python
signatures and results. `report` is an optional `tlor.Report` (§7).

```python
plan = tlor.schedule(instance, fleet, state, start, horizon, method, params=None, weights=None, report=None)

routes = tlor.route(instance, fleet, day, points, loads_kg, method, params=None, weights=None, report=None)

solution = tlor.improve(instance, fleet, state, solution, route_method, method,
                        route_params=None, params=None, weights=None, report=None)

end_state, levels = tlor.simulate(instance, state, start, days, plan, demand_kg=None)

evaluation = tlor.evaluate(instance, fleet, state, solution, demand_kg=None, params=None)

violations = tlor.check(instance, fleet, state, solution, demand_kg=None)

result = tlor.run_rolling(instance, fleet, state=None, *, first, last, horizon, step, schedule, route,
                          improve=None, weights=None, evaluation=None, check_each_step=False,
                          realized_demand_kg=None, report=None)
```

- `route` takes the points as a sequence of node indices and `loads_kg` as the
  same number of masses.
- `simulate` returns the state after the last simulated day and a
  `(days, node_count)` array: row `i` holds the level of every container on
  day `start + i`, before that day's collection.
- `evaluate` returns an `Evaluation`: `total` is a dict of the 14 indicators,
  `per_day` a list of such dicts (index 0 is day `start`), `start` the first
  day, and `objective(weights)` the weighted sum of the total.
- `check` returns a list of `Violation(kind, day, route, point, message)`;
  `route` and `point` are `-1` when the violation is not about one route or
  one point.
- `run_rolling` returns a `RollingResult` with `solution`, `evaluation`,
  `violations` and `end_state`. `schedule`, `route` and `improve` are each a
  method name or a `(name, params)` pair; `evaluation` is the evaluation
  params dict.
- `tlor.indicator_names()` returns the 14 indicator names in a fixed order.

## 5. Methods

```python
tlor.methods(step)   # step is "schedule", "route" or "improve"
```

returns a list of `MethodInfo(name, description, params)`, each parameter a
`ParamSpec(name, type, default, min, max, description)` with `type` one of
`"int"`, `"double"`, `"bool"`, `"string"` and `None` for a missing bound. The
built-in methods are described in [Built-in methods](../methods/).

## 6. Files

```python
tlor.write_solution(out_dir, instance, solution, state=None, demand_kg=None, first_date=None)
tlor.write_routes_json(path, routes)
```

`write_solution` creates `out_dir` when needed, removes the
`routes_<date>.json` files an earlier run left there, and writes one
`routes_<date>.json` per routed day, `collection_schedule.csv` and
`route_statistics.csv` (see [Files](../files-and-cli/#2-output-files)). The
collected masses come from simulating the plan from `state` (the state at the
plan's first day, initial by default) with `demand_kg`. Dates count from
`first_date` (a `datetime.date` or `"YYYY-MM-DD"`), by default the
instance's own; an instance built from arrays needs it.

## 7. Time report

```python
from pathlib import Path

Path("local/runs").mkdir(parents=True, exist_ok=True)
with tlor.Report("local/runs/report.json", experiment="baseline", author="me") as report:
    result = tlor.run_rolling(inst, fleet, first=0, last=20, horizon=7, step=1,
                              schedule="threshold", route="nearest_neighbour", report=report)
```

`Report(path=None, **context)` creates a report; the keyword arguments are
free text stored in its context. Used in a `with` block with a path, it is
written when the block ends; otherwise call `report.write(path)`. Inside a
method, `report.phase(name)` (a context manager) times a section and
`report.count(name, delta=1)` adds to a counter. The directory of the path
must exist. See [Time report](../report/) for what the file holds.

## 8. Writing a method in Python

A method is a function that takes a **request** and returns the result of its
step. Register it under a name, and every call of that step can use it, in
`run_rolling`, in `improve` and from `tlor.route` and the like:

```python
tlor.register_method(step, name, fn, params=(), description="")
tlor.unregister_method(step, name)
```

`params` is a list of `tlor.ParamSpec`; the library checks the values callers
pass against it and fills the defaults, so `req.params` always holds every
declared parameter. A name already registered in the step is refused; built-in
methods cannot be unregistered. Registrations last until the Python process
ends.

| Step | `fn` receives | `fn` returns |
| --- | --- | --- |
| `"schedule"` | `ScheduleRequest(instance, fleet, state, start, horizon, params, weights, report)` | a `Plan(start, horizon)` |
| `"route"` | `RouteRequest(instance, fleet, day, points, loads_kg, params, weights, report)` | a `DayRoutes(day)` |
| `"improve"` | `ImproveRequest(instance, fleet, state, solution, route_method, route_params, params, weights, report)` | a `Solution` over the same days as `solution` |

`params` and `weights` are dicts, `points` an `int32` array and `loads_kg` a
`float64` array of the same length, `report` a `tlor.Report` or `None`.
`horizon` is already cut at the last day of the data.

### 8.1 A route method

Visits the points in index order, closing the route when the next point would
not fit in the truck, and spreading routes over the working trucks:

```python
import numpy as np
import tlor


def in_order(req):
    depot = req.instance.depot
    capacity = req.fleet.capacity_kg
    trucks = np.flatnonzero(req.fleet.available[req.day]).tolist() or [0]
    limit = req.params["max_points"]
    routes = tlor.DayRoutes(req.day)
    route, load, index = [], 0.0, 0
    for point, kg in zip(req.points.tolist(), req.loads_kg.tolist()):
        if route and (load + kg > capacity[trucks[index]] or len(route) == limit):
            routes.add(trucks[index], [depot, *route, depot])
            route, load, index = [], 0.0, (index + 1) % len(trucks)
        route.append(point)
        load += kg
    if route:
        routes.add(trucks[index], [depot, *route, depot])
    return routes


tlor.register_method(
    "route", "in_order", in_order,
    params=[tlor.ParamSpec("max_points", "int", 20, min=1, description="Most points on one route")],
    description="Visits the points in index order, one truck after the other",
)
```

It is then used like any other:

```python
result = tlor.run_rolling(inst, fleet, first=0, last=13, horizon=7, step=1,
                          schedule="threshold", route=("in_order", {"max_points": 12}))
```

### 8.2 A schedule method

Collects every active point every `k` days, on working days:

```python
def every_k_days(req):
    plan = tlor.Plan(req.start, req.horizon)
    working = req.fleet.available.any(axis=1)
    points = np.flatnonzero(req.instance.active)
    count = 0
    for day in range(req.start, req.start + req.horizon):
        if day % req.params["k"] == 0 and working[day]:
            for point in points:
                plan.add(day, int(point))
                count += 1
    if req.report is not None:
        req.report.count("collections", count)
    return plan


tlor.register_method("schedule", "every_k_days", every_k_days,
                     params=[tlor.ParamSpec("k", "int", 3, min=1, description="Days between two rounds")])
```

### 8.3 An improve method

`req.solution` is a copy that belongs to the method. Change it or build a new
one, route the days you changed with `req.route_method`, and return it. This
one drops collections below a fill rate, then keeps the result only when the
objective improves:

```python
def remove_low_fill(req):
    inst, plan = req.instance, req.solution.plan
    start, horizon = plan.start, plan.horizon
    _, levels = tlor.simulate(inst, req.state, start, horizon, plan)
    for day, points in plan.days().items():
        for point in points:
            if levels[day - start, point] / inst.capacity_kg[point] < req.params["min_fill"]:
                plan.remove(day, point)
    candidate = tlor.Solution(plan)
    _, levels = tlor.simulate(inst, req.state, start, horizon, plan)
    for day in range(start, start + horizon):
        points = plan.points(day)
        if len(points):
            candidate.set_routes(tlor.route(inst, req.fleet, day, points, levels[day - start, points],
                                            req.route_method, req.route_params, req.weights, req.report))
        else:
            candidate.set_routes(tlor.DayRoutes(day))

    def score(solution):
        return tlor.evaluate(inst, req.fleet, req.state, solution).objective(req.weights)

    return candidate if score(candidate) < score(req.solution) else req.solution


tlor.register_method("improve", "remove_low_fill", remove_low_fill,
                     params=[tlor.ParamSpec("min_fill", "double", 0.3, min=0, max=1)])
```

Note `levels[day - start, ...]`: the rows of `simulate` count from its start
day.

### 8.4 Rules for methods

- **Return the right range.** A schedule method returns a plan of exactly
  `(req.start, req.horizon)`, a route method routes of `req.day`, an improve
  method a solution over the plan's days; anything else fails with
  `InternalError`.
- **Best effort.** Return something even when the constraints cannot all be
  met; `check` reports what is broken. Raise only when no result can be
  built.
- **Errors.** An exception raised by the method, or a wrong return type, makes
  the step fail with `tlor.CallbackError`, whose message starts with the
  exception's type (`ValueError: …`). The registry and later calls keep
  working.
- **Borrowed objects.** `req.instance`, `req.fleet`, `req.state` and
  `req.report` belong to the caller and are only valid during the call: keep
  the arrays you read from them (they are copies), not the objects. Using one
  after the method returned raises `InvalidArgument`.
- **Speed.** Each call crosses from C++ into Python. That is negligible for a
  schedule method or a route method called once a day, but `shift_day` calls
  the route method for every move it tries.
- **Randomness.** Take a `seed` parameter and use `np.random.default_rng(seed)`
  so a run can be repeated.

## 9. Errors

Every failure raises a subclass of `tlor.Error`, with `status` (the numeric
code) and `message`:

| Exception | Status |
| --- | --- |
| `tlor.InvalidArgument` | `invalid_argument` |
| `tlor.UnknownMethod` | `unknown_method` |
| `tlor.InvalidParam` | `invalid_param` |
| `tlor.IOFailure` | `io_error` |
| `tlor.Infeasible` | `infeasible` |
| `tlor.CallbackError` | `callback_error` |
| `tlor.InternalError` | `internal_error` |

```python
try:
    tlor.run_rolling(inst, fleet, first=0, last=6, horizon=7, step=1,
                     schedule=("threshold", {"threshold": 1.5}), route="nearest_neighbour")
except tlor.InvalidParam as error:
    print(error)   # parameter 'threshold' of method 'threshold' is out of range
```

Inputs are checked before any work starts, so a mistake fails fast.

## 10. Copies and lifetime

- The library's objects are freed when Python garbage-collects them; there is
  nothing to close.
- `copy.copy` and `copy.deepcopy` make real, independent copies of `Plan`,
  `DayRoutes`, `Solution`, `State` and `Params`. A copy of an `Instance`, a
  `Fleet` or an `Evaluation` is the same object: their data cannot change
  (only an instance's attributes can, and that change is seen through every
  name bound to it).
- These objects cannot be pickled, so they cannot be sent to another process
  with `multiprocessing`. Send the arrays and rebuild the objects there.
- The library is not thread-safe: do not call it from several threads at
  once.
