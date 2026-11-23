---
title: Time report
description: Measure where the time goes in a run, add phases and counters to a method, and read the JSON report.
sidebar:
  order: 9
---

A report records, for each call into the library, how long it took and where
the time went, together with the parameters and the result. It is the tool
to compare two methods or two settings on the same data.

## 1. Turning it on

Create a report and pass it to any step or to `run_rolling`; leave it out
(`None`, `nullptr`, `NULL`) and nothing is measured.

```python
from pathlib import Path

Path("local/runs/baseline").mkdir(parents=True, exist_ok=True)
with tlor.Report("local/runs/baseline/report.json", experiment="baseline") as report:
    result = tlor.run_rolling(inst, fleet, first=0, last=62, horizon=7, step=1,
                              schedule="threshold", route="nearest_neighbour", report=report)
```

- Python: `tlor.Report(path, **context)`, written at the end of the `with`
  block, or by `report.write(path)`.
- C++: `tlor::Report report;`, then `report.set_context(key, value)` and
  `report.write_json(path)`.
- C: `tlor_report_create`, `tlor_report_set_context`, `tlor_report_write_json`.
- Command line: `--report FILE`.

The directory of the file must exist (`TLOR` creates `--out` first, so a
report inside it works). Time is measured with a monotonic clock, in
nanoseconds; measure with a **release** build, since a debug build is several
times slower.

## 2. What it records

- **Calls.** Each call you make (a step, or `run_rolling`) is one record: the
  step, the method, the parameters with their defaults filled in, the seed
  when the method has one, the size of the instance, the status, the wall
  time, and, for calls that produce a solution, its indicators.
- **Phases.** Inside a call, time is split into named phases, nested, and
  aggregated by path: a phase entered 63 times appears once with
  `count: 63` and the total, smallest and largest time. Calls made by the
  library inside your call (the steps inside `run_rolling`, the routing
  inside `improve`) appear as phases named `call:<step>/<method>`.
- **Counters.** Named integers attached to the current phase, for things
  counted in loops: moves tried, candidates examined. `run_rolling` with
  `check_each_step` adds `violations:<kind>` counters.

The phases of the built-in pieces:

| Where | Phases and counters |
| --- | --- |
| `run_rolling` | `schedule`, `route`, `improve` (when given), `simulate`, `check`, `evaluate` |
| `threshold` | `due_days`, counter `collections` |
| `nearest_neighbour` | `construction`, counter `candidates_evaluated` |
| `shift_day` | `initial_routing`, `search` with counters `moves_evaluated` and `moves_accepted` |

## 3. Adding phases to a method

Use phases for a few coarse sections of a method, and counters inside loops:
a phase costs two clock readings, a counter an addition.

```python
def my_route(req):
    report = req.report                       # None when the caller did not ask for one
    if report is None:
        return build(req)
    with report.phase("construction"):
        routes = build(req)
    with report.phase("two_opt"):
        routes, moves = two_opt(req, routes)
        report.count("moves_tried", moves)
    return routes
```

```cpp
DayRoutes route_my_method(const RouteRequest& request) {
    Phase construction(request.report, "construction");   // does nothing when report is null
    // …
    count(request.report, "moves_tried", moves);
}
```

A phase left open when a method returns is closed by the library and counted
in an `unclosed_phases` counter, so a forgotten `end` shows in the report
without breaking it.

## 4. The file

The report is one JSON file, schema version 1:

```json
{
  "schema_version": 1,
  "context": {
    "library_version": "0.1.0", "git_commit": "7739750", "build_type": "Release",
    "compiler": "Clang 23.1.3", "os": "Linux", "started_at": "2026-11-23T15:17:30Z",
    "user": {"experiment": "baseline"}
  },
  "phases": [],
  "counters": {},
  "calls": [
    {
      "step": "rolling",
      "method": "threshold+nearest_neighbour",
      "params": {"first_day": 0, "last_day": 62, "horizon": 7, "step": 1, "check_each_step": false,
                 "schedule.threshold": 0.85, "schedule.use_point_threshold": true,
                 "route.criterion": "distance"},
      "seed": null,
      "instance": {"nodes": 329, "active_points": 300, "days": 63, "trucks": 6},
      "status": "ok",
      "wall_ns": 3444910,
      "phases": [
        {"name": "schedule", "count": 63, "total_ns": 681943, "min_ns": 3560, "max_ns": 13140,
         "counters": {},
         "children": [
           {"name": "call:schedule/threshold", "count": 63, "total_ns": 633853, "min_ns": 2850,
            "max_ns": 12430, "counters": {},
            "children": [{"name": "due_days", "count": 63, "total_ns": 623723, "min_ns": 2690,
                          "max_ns": 12290, "counters": {"collections": 7091}, "children": []}]}
         ]},
        {"name": "route", "count": 63, "total_ns": 1557809, "...": "…"},
        {"name": "simulate", "count": 63, "total_ns": 211938, "...": "…"},
        {"name": "check", "count": 1, "total_ns": 468530, "...": "…"},
        {"name": "evaluate", "count": 1, "total_ns": 456121, "...": "…"}
      ],
      "counters": {},
      "series": {},
      "indicators": {"distance_m": 5722001.735, "routes": 79.0, "collections": 1099.0, "...": "…"}
    }
  ]
}
```

This is the report of the `TLOR` command shown in
[Files and the TLOR program](../files-and-cli/#3-the-tlor-program), trimmed
(`…`): the whole run took 3.4 ms in a release build, and the `threshold`
method 0.6 ms of it, over 63 calls. Its `collections` counter (7091) counts
every collection planned in every window, most of them in days a later
window planned again; the indicator (1099) counts those finally kept.

| Field | Meaning |
| --- | --- |
| `context` | The build (version, git commit, build type, compiler, OS), the start time in UTC, and your free `user` keys. `TLOR` adds `command_line` and `first_date`. |
| `phases`, `counters` (top level) | Phases and counters opened outside any call. |
| `calls[].step`, `method` | The step (`schedule`, `route`, `improve`, `rolling`) and method. For `rolling`, the methods joined with `+`, and the parameters prefixed with their step (`schedule.threshold`) plus `weight.<name>` for the weights. |
| `calls[].seed` | The `seed` parameter when the method has one, else `null`. |
| `calls[].status` | `ok`, or the error name when the call failed. |
| `calls[].wall_ns` | Duration of the whole call. |
| `calls[].phases[]` | `name`, `count`, `total_ns`, `min_ns`, `max_ns`, `counters`, `children`. |
| `calls[].series` | Reserved for time series of values (empty in version 1). |
| `calls[].indicators` | The 14 indicators of the result, or `null` when the call produces none or failed. |

## 5. Comparing runs

Reports are plain JSON, so a few lines of Python turn a directory of them
into a table:

```python
import csv
import json
from pathlib import Path

rows = []
for path in sorted(Path("local/runs").glob("*/report.json")):
    report = json.loads(path.read_text())
    for call in report["calls"]:
        rows.append({
            "run": path.parent.name,
            "experiment": report["context"]["user"].get("experiment", ""),
            "method": call["method"],
            "seconds": call["wall_ns"] / 1e9,
            **(call["indicators"] or {}),
        })

with open("local/runs/experiment_results.csv", "w", newline="") as out:
    writer = csv.DictWriter(out, fieldnames=sorted({key for row in rows for key in row}))
    writer.writeheader()
    writer.writerows(rows)
```
