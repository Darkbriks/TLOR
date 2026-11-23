---
title: Built-in methods
description: The methods shipped with the library, threshold, nearest_neighbour, greedy and shift_day, their exact rules and their parameters.
sidebar:
  order: 4
---

The library ships one reference method per step, plus a second route
method, `greedy`. Together they give a complete, checkable solution on the
real data, serve as the baseline other methods are compared with, and show
how a method is written. The owners of the schedule, routing and improvement
roles take them over: tune them, replace them, or add better methods next to
them.

All four are deterministic: the same inputs and parameters give the same
result, and ties are broken by the smaller node index.

To list the methods of a step and their parameters at run time, including
those registered by other people:

```python
for method in tlor.methods("route"):
    print(method.name, "-", method.description)
    for p in method.params:
        print("   ", p.name, p.type, "default", p.default, "min", p.min, "max", p.max, "-", p.description)
```

The `TLOR` program has no listing option; the tables below are the reference.

## 1. `threshold` (schedule)

Collects each point when its predicted level reaches a fill threshold.

| Parameter | Type | Default | Bounds | Meaning |
| --- | --- | --- | --- | --- |
| `threshold` | double | `0.85` | `(0, 1]` | Fill rate at which a point is due. |
| `use_point_threshold` | bool | `true` | | Use the instance's `threshold` attribute for the points where it is set (from `point_policy.csv`), instead of the parameter. |

For each active point, the method walks the horizon day by day from the
start state, adding the predicted demand:

1. The point is **due** on the first day `t` where its level, that day's
   waste included, reaches `threshold × capacity_kg`, or where
   `t − last_collection` reaches its `max_days` attribute when that is set.
2. It is collected on the **latest day up to `t`**, after its previous
   collection, when at least one truck works; when there is none, on the
   **first working day after `t`** within the horizon. When no working day is
   left, it is not collected in this horizon.
3. Its level resets to zero after the collection, and the walk continues to
   the end of the horizon, so a point can be collected several times.

It neither balances days nor checks the fleet's capacity: on a busy day it
plans more than the trucks can carry, and `check` says so.

Report counter: `collections` (number of collections planned), in the phase
`due_days`.

## 2. `nearest_neighbour` (route)

Builds routes greedily, always driving to the nearest point that still fits.

| Parameter | Type | Default | Meaning |
| --- | --- | --- | --- |
| `criterion` | string | `distance` | `distance` or `duration`: what "nearest" means. |

1. The trucks available that day are taken in index order. Each route starts
   at the depot and repeatedly goes to the nearest unvisited point, measured
   **from the current node** (the matrices are asymmetric), among those whose
   load fits in the truck's remaining capacity and whose visit still lets the
   truck get back to the depot and unload within its `max_work_s` for the
   day. When none fits, the route returns to the depot.
2. The same truck starts a new route while it can still serve a remaining
   point within its time; then the next truck takes over.
3. When every available truck is used up and points remain (or when no truck
   works that day), routes are still built, each given to the truck with the
   least work so far, ignoring the time limit. The result is complete but
   breaks constraints, and `check` reports `work_time`, `capacity` or
   `truck_unavailable`.

The time limit always uses durations, whatever the criterion.

Report counter: `candidates_evaluated` (points examined while choosing the
next one), in the phase `construction`.

## 3. `greedy` (route)

The first routing heuristic of the project, written to get an upper bound on
the distance of a day: a nearest-neighbour tour built for a single truck,
with its capacity as the only limit. It has no parameter.

1. Every tour is driven by the **first truck available** that day (truck 0
   when none is), and filled up to that truck's capacity.
2. From the depot, the truck repeatedly goes to the nearest unvisited point,
   by distance from the current node (ties to the smaller index), among those
   whose load still fits. When none fits, it returns to the depot and starts
   a new tour.
3. A point heavier than the truck on its own gets a tour of its own.

Unlike `nearest_neighbour`, it ignores the work time and the other trucks:
on a busy day, its tours exceed the truck's `max_work_s`, and `check` reports
`work_time`. When the work time of that first truck never binds, and no
point is heavier than the truck, `nearest_neighbour` builds the same tours:
both give the 934.4 km below.

Report phase: `construction`.

The `greedy_bound` program, built with every preset, runs it on all active
points with every container full and one 10 t truck, and prints the number of
tours and their distance: 14 tours and 934.4 km on the project data. It
finds `data/` and `Maps/` from any directory inside the repository:

```bash frame="terminal"
./build/linux-release/bin/greedy_bound
```

## 4. `shift_day` (improve)

Local search that moves one collection of one point to a nearby day when this
lowers the objective.

| Parameter | Type | Default | Bounds | Meaning |
| --- | --- | --- | --- | --- |
| `max_iterations` | int | `1000` | `≥ 0` | Maximum number of moves evaluated. |
| `max_shift` | int | `1` | `≥ 1` | Largest shift of a collection, in days. |
| `seed` | int | `0` | `≥ 0` | Seed of the order in which moves are tried. |

1. Days of the solution that are planned but not routed are routed first,
   with the route method given to `improve`.
2. A **move** takes one collection of one point and shifts it by at most
   `max_shift` days, within the plan's days, onto a day with at least one
   truck available and where that point is not already collected.
3. The moves are tried in an order shuffled with the seed. For each move, the
   loads are recomputed with `simulate` on the predicted demand, every day
   whose points or loads changed is routed again, and the objective is
   computed with the weights.
4. The first move that **strictly lowers** the objective is kept, and a new
   pass starts. The search stops after a pass without improvement, or after
   `max_iterations` evaluated moves.

:::caution[It needs weights]
With no weights the objective is always 0, so no move is ever kept and the
solution comes back unchanged. Pass at least one weight, such as
`{"distance_m": 1, "overflow_point_days": 1e6}`.
:::

Each move routes the changed days again, so the method is slow with a route
method written in Python. On the real data, day by day over 63 days with
`nearest_neighbour` and a release build, one run took about 14 s with the
default parameters (2 s week by week), against 0.01 s without improvement.

Report phases and counters: `initial_routing`, then `search` with
`moves_evaluated` and `moves_accepted`; the routing calls appear inside as
`call:route/<method>`.

## 5. Choosing parameters

Parameters are checked before the method runs: an unknown name, a wrong type
or a value out of bounds fails with `invalid_param`, and the message names
the parameter and the method. In Python, pass them as a dict:

```python
plan = tlor.schedule(inst, fleet, None, 0, 7, "threshold",
                     params={"threshold": 0.9, "use_point_threshold": False})
```

From the command line, use `--param schedule.threshold=0.9`. To write your
own method, see [Python package](../python/#8-writing-a-method-in-python) or
[C++](../cpp/#3-adding-a-built-in-method).
