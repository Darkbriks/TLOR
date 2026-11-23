---
title: Files and the TLOR program
description: The data files the library reads, the files it writes, and every option of the TLOR command-line program.
sidebar:
  label: Files and the TLOR program
  order: 8
---

## 1. Input files

`load_instance` (Python: `tlor.Instance.load`) reads two directories: the
**tables** (`data/tables/`, produced by step 1) and the **map data**
(`Maps/offline map/data/`, from the course). The tables are CSV files with
`;` as separator:

- columns are found by **name**, in any order, and other columns are
  ignored; spaces around names and values are dropped;
- Windows line endings, a UTF-8 byte order mark and blank lines are
  accepted; quoted fields are not;
- numbers use a `.` for decimals; booleans are `True` or `False` (or in
  lower case); dates are `YYYY-MM-DD`.

| File | Columns used | Rules |
| --- | --- | --- |
| `active_points.csv` | `point_id`, `capacity_kg`, `is_depot`, `active` | One row per node; the ids are `0 … rows − 1`, each once. An empty `capacity_kg` means 0. Exactly one row has `is_depot` true. An active point needs a positive capacity; the depot must not be active. |
| `daily_demand.csv` | `point_id`, `date`, `daily_kg` | Day 0 is the earliest date. The dates must have no gap. Only active points may appear, at most once per date; a point-day without a row has no waste. |
| `point_policy.csv` | `point_id`, `threshold` | Read unless the caller turns it off. Becomes the `threshold` attribute; points without a row, or with an empty value, have none. |
| `distances.json`, `durations.json` | (map data) | A JSON list of `node_count` lists of `node_count` numbers, the origin first. |
| service-time CSV | `service_time` | `node_count` values, in node order. By default `exemple_points_service_time.csv` in the map data directory. |

A missing file is an `io_error` naming its path; anything malformed is an
`invalid_argument` naming the file, the line and the column:
`daily_demand.csv line 2 column daily_kg: 'ten' is not a number`. Two
mistakes are caught on purpose: a date with a typo (`9023-04-15`) is reported
as a gap in the dates, and a demand row for an inactive point is refused
rather than ignored.

## 2. Output files

`write_solution` (Python: `tlor.write_solution`) writes into a directory,
created when needed:

| File | Content |
| --- | --- |
| `routes_<date>.json` | One per day that has routes in the solution: a JSON list of routes, each a list of node indices from the depot back to the depot, as in `exemple_routes.json`. A day routed with no route at all (no collection, no truck) gets `[]`. The map tools draw these files as they are. |
| `collection_schedule.csv` | `date;point_id;collected_kg;fill_rate`: one row per planned collection of an active point, by date then point. `collected_kg` is the level of the container that day, from simulating the plan with the given demand; `fill_rate` is `collected_kg / capacity_kg`. |
| `route_statistics.csv` | `date;route;truck;distance;duration;load_kg;points`: one row per route. `route` is its index within the day, `distance` in metres, `duration` in seconds (driving, service at each point, and unloading at the depot), `load_kg` the mass collected, `points` the number of distinct points visited. A point visited by two routes the same day counts in the load of the first. |

Before writing, it removes the `routes_<date>.json` files left in the
directory by an earlier run, so the directory never mixes two runs; other
files are left alone. Numbers are written in their shortest exact form, with
a `.` for decimals.

## 3. The `TLOR` program

`TLOR` loads the data, runs the [rolling driver](../steps/#3-the-rolling-driver),
writes the solution, and prints the indicators:

```bash frame="terminal"
./build/linux-release/bin/TLOR --tables data/tables --maps "Maps/offline map/data" \
    --out local/runs/baseline --trucks 6 --truck-capacity-kg 8000 --truck-max-work-s 28800 \
    --weekends-off --report local/runs/baseline/report.json
```

### 3.1 Options

| Option | Default | Meaning |
| --- | --- | --- |
| `--tables DIR` | required | Directory of the CSV tables. |
| `--maps DIR` | required | Directory of `distances.json` and `durations.json`. |
| `--out DIR` | required | Where the solution files go. |
| `--trucks N` | required | Number of trucks, at least 1. |
| `--truck-capacity-kg X` | required | Capacity of each truck. |
| `--truck-max-work-s X` | required | Work time of each truck per day. |
| `--service-time FILE` | `exemple_points_service_time.csv` in `--maps` | Service-time CSV. |
| `--first D` | `0` | First day to plan, as a day index or a `YYYY-MM-DD` date. |
| `--last D` | the last day | Last day to plan. |
| `--horizon H` | `7` | Days planned by each window. |
| `--step K` | `1` | Days kept from each window. |
| `--schedule NAME` | `threshold` | Schedule method. |
| `--route NAME` | `nearest_neighbour` | Route method. |
| `--improve NAME` | none | Improve method. |
| `--param STEP.NAME=VALUE` | | A method parameter; `STEP` is `schedule`, `route`, `improve` or `evaluation`. Repeatable. |
| `--weight NAME=VALUE` | | An objective weight on an indicator. Repeatable. |
| `--weekends-off` | | No truck works on Saturdays and Sundays, from the dates of the data. |
| `--report FILE` | | Write the time report there. |
| `--check-each-step` | | Check each window's decisions as they are made, and count the violations in the report. |
| `--help`, `-h` | | Print the usage. |

A `VALUE` is read as an integer when it is one, else as a number, else as
`true` or `false`, else as text: `--param route.criterion=duration`,
`--param improve.max_iterations=500`, `--weight overflow_point_days=1e6`.
`improve.` parameters need `--improve`. Each single-valued option may
appear once. With `--first` later than day 0,
the containers start empty that day (see
[Starting later than day 0](../steps/#31-starting-later-than-day-0)).

Every truck has the same capacity and work time from the command line; for a
mixed fleet or other non-working days, use Python or C++.

### 3.2 Output and exit codes

On success, `TLOR` prints one line per indicator, then the number of
violations, in total and per kind:

```text
distance_m 5722001.735
…
workload_imbalance_s 40048.6
violations 0
```

and exits with `0`, **even when there are violations**: read the
`violations` lines. It exits with `2` on a command-line mistake (unknown
option, missing value, a value that is not a number), printing the usage,
and with `1` when the library rejects the data or a parameter, printing the
error:

```text
TLOR: invalid_param: parameter 'threshold' of method 'threshold' is out of range
```
