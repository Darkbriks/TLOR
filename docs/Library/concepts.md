---
title: Concepts
description: The model the library works on, days, containers and the fill convention, trucks, plans, routes and solutions, parameters, indicators and the objective.
sidebar:
  order: 2
---

Every interface of the library (Python, C++, C, command line) works on the
same objects. This page defines them once; the other pages only say how to
build and use them.

## 1. Units and indices

- Distances in **metres**, durations in **seconds**, waste in **kilograms**,
  fill rates as **fractions** from 0 to 1. The library never converts units.
- A **node** is an index `0 … node_count − 1`. One node is the **depot**; the
  others are collection **points**. With the project data there are 329 nodes
  and the depot is `328`, but the library takes both from the data and never
  assumes them.
- A **day** is an index `0 … day_count − 1`. Day 0 is the first date of the
  demand table (2023-04-14 with the project data). The library itself has no
  calendar: only the file loaders and writers convert days to dates.

## 2. Instance

An instance is the fixed data of one problem. It is copied when built and
never changes afterwards, except for its named attributes.

| Field | Shape | Meaning |
| --- | --- | --- |
| `distance_m` | node × node | Road distance from a node (row) to another (column). **Asymmetric**: `distance_m[a][b]` and `distance_m[b][a]` differ, so reversing a route changes its length. |
| `duration_s` | node × node | Driving time, same layout. |
| `service_s` | node | Time spent at a point to empty it. The **depot's** value is the unloading time paid at the end of each route. |
| `capacity_kg` | node | Container capacity. Must be positive for active points. |
| `active` | node | Whether the point is collected at all. Inactive points (the 28 points left out of the project, and the depot) never appear in a plan or a route. |
| `demand_kg` | day × node | **Predicted** waste added to each container each day. |
| `depot` | — | Index of the depot. |

**Attributes** are optional per-node arrays attached to an instance by name,
so a method can receive new data without any change to the library. A value
of NaN means "not set for this node". Two names have a meaning:

- `threshold`: the fill rate at which the `threshold` method collects a point
  (loaded from `point_policy.csv`);
- `max_days`: the longest gap, in days, allowed between two collections of a
  point. The `threshold` method respects it and the checker reports any gap
  longer than it.

Any other name is free for your own methods.

## 3. Containers: state and fill convention

A **state** gives, for every node, the waste in its container in kg at the
start of a day (`level_kg`) and the last day it was collected
(`last_collection`, `-1` when never). The **initial state** has empty
containers never collected. A state belongs to a day: it must not record a
collection on or after the day it is used from.

Each day `t`, for each point `i` (the same convention as step 1's
simulation):

1. the day's waste is added first: `level = level_kg[i] + demand[t][i]`;
2. if `i` is collected on `t`, the truck takes `level` kg and the container is
   emptied; otherwise it keeps `level`, **without any cap**;
3. the **fill rate** is `level / capacity_kg[i]`, and the point-day
   **overflows** when `level > capacity_kg[i]`.

So the mass collected at a point is everything accumulated since its last
collection, including the day of the collection itself. `simulate` applies
this rule; the evaluation and the checker apply it too, each on its own.

## 4. Fleet

A fleet is a list of trucks, each with a `capacity_kg` (load per route) and a
`max_work_s` (work time per day), and an availability table
`available[day][truck]`. A day where no truck is available is a non-working
day: weekends, for instance, are expressed this way.

A truck may drive **several routes in one day**, emptying its load at the
depot between them. Its capacity limits each route; its `max_work_s` limits
the sum of its routes that day.

The **work time** of a route is its driving time (`duration_s` along the
route), plus the service time of every point it visits, plus the depot's
service time once at the end.

## 5. Plan, routes, solution

- A **plan** covers the days `start … start + horizon − 1` and gives, for
  each day, the set of points collected that day. Points are kept sorted and
  unique.
- A **route** is a truck index and a list of nodes that starts and ends at
  the depot, as in the team's data contract: `[328, 0, 2, 3, 328]`.
- **DayRoutes** are the routes of one day.
- A **solution** is a plan plus the DayRoutes of each of its days. A day may
  have no routes yet (not routed). Replacing the plan drops the routes of days
  that are no longer in it.

Methods always return something, even when they cannot satisfy every
constraint: a plan that exceeds the fleet, routes on a day without trucks.
Judging the result is the job of `check`.

## 6. Parameters and weights

Each method declares its **parameters**: a name, a type (`int`, `double`,
`bool` or `string`), a default, optional inclusive bounds and a description.
When a step runs a method, the library checks the parameters you pass
against these declarations: an unknown name, a wrong type or a value out of
bounds is an `invalid_param` error, and missing parameters get their default.
An integer is accepted where a `double` is declared. The method then receives
the complete, checked set.

**Weights** use the same container, but map indicator names to numbers. They
define the objective (below) and are passed to every method, which may use
them or not.

## 7. Indicators and objective

`evaluate` computes these indicators for each day and in total, from a
solution and a demand (predicted or realized):

| Indicator | Definition |
| --- | --- |
| `distance_m` | Sum of `distance_m` along every route. |
| `travel_s` | Sum of `duration_s` along every route. |
| `service_s` | Service time of every visited point, plus the depot's at the end of each route. |
| `work_s` | `travel_s + service_s`. |
| `overtime_s` | Sum over trucks and days of `max(0, work of the truck that day − max_work_s)`. |
| `routes` | Number of routes. |
| `trucks_used` | Sum over days of the number of distinct trucks that drive. |
| `collections` | Number of planned collections of active points. |
| `collected_kg` | Mass collected. |
| `overflow_point_days` | Point-days where `level > capacity_kg`. |
| `overflow_kg_days` | Sum over point-days of `max(0, level − capacity_kg)`. |
| `low_fill_collections` | Collections whose fill rate is below `low_fill_threshold` (an evaluation parameter, default `0.5`). |
| `mean_fill_at_collection` | Mean fill rate of the collected points. In the total, the mean over every collection. |
| `workload_imbalance_s` | Largest minus smallest daily `work_s`, over the days with at least one truck available. Only in the total; `0` in each day. |

The **objective** is the weighted sum
`Σ weights[name] × indicators[name]`, over the names present in the weights.
A name that is not an indicator, or a weight that is not a number, is an
`invalid_param` error. Lower is better: a method that minimizes the objective
with `{"distance_m": 1, "overflow_point_days": 1e6}` travels as little as
possible while treating each overflow as a 1000 km detour.

## 8. Violations

`check` returns every broken constraint, each with a **kind**, the day, the
route index (`-1` when the violation is not about one route), the point
(`-1` when it is not about one point) and a message.

| Kind | Reported when |
| --- | --- |
| `route_endpoints` | A route does not start and end at the depot, or goes through the depot in the middle. |
| `visit_count` | A planned point is not visited exactly once that day (including a planned day with no routes at all). |
| `unplanned_visit` | A route visits a point that is not planned that day. |
| `invalid_node` | A plan or a route holds an index out of range or a node that is not an active point. |
| `capacity` | The load of a route exceeds the capacity of its truck. |
| `work_time` | The work of a truck in a day exceeds its `max_work_s`. |
| `truck_unavailable` | A route uses a truck that does not exist or is not available that day. |
| `overflow` | A container overflows, with the demand given to `check`. |
| `max_days` | A point goes longer than its `max_days` attribute without collection. |

The checker shares no code with the methods, the simulation or the
evaluation, so a bug in one of them cannot hide itself. Which kinds are
acceptable is up to you: an overflow may be tolerated and penalized through
the weights, a route through an inactive point never should.

## 9. Errors

Every interface reports failures with the same codes:

| Status | Meaning |
| --- | --- |
| `invalid_argument` | An input is malformed or inconsistent: wrong array size, day out of range, malformed data file. |
| `unknown_method` | No method of that name in that step. |
| `invalid_param` | A parameter, weight or evaluation parameter is unknown, of the wrong type or out of bounds. |
| `io_error` | A file cannot be read or written. The message holds its path. |
| `infeasible` | A method could not build any result at all. |
| `callback_error` | A method written in Python or C failed. The message holds its error. |
| `internal_error` | A bug in the library, or a method that returned a result for the wrong days. |

The message always says what was wrong, with the file, line and column for
data files. The [Python](../python/#9-errors), [C++](../cpp/#4-errors) and
[C](../c-api/#2-status-codes-and-errors) pages say how each language exposes
them.
