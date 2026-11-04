# Proposed Team Roles

The project can be divided into four parallel workstreams. The key principle is that every participant starts with either the existing files or a small synthetic dataset. Nobody needs to wait for the final cleaned dataset or final algorithm.

Before starting, the team should agree on a small common data contract:

| Field            | Description                                  |
| ---------------- | -------------------------------------------- |
| `point_id`       | integer from 0 to 327                        |
| `depot_id`       | 328                                          |
| `date`           | `YYYY-MM-DD`                                 |
| `current_kg`     | current amount of waste                      |
| `capacity_kg`    | maximum capacity of the collection point     |
| `daily_kg`       | predicted daily waste generation             |
| `distance[i][j]` | road distance in metres                      |
| `duration[i][j]` | travel time in seconds                       |
| `route`          | `[328, point_1, point_2, ..., 328]`          |

This agreement should take no more than one meeting. After that, everyone can work independently.

---

## Role 1 — Data and Demand Forecasting Specialist

### Main objective

Prepare reliable input data and estimate how much waste is generated at each collection point over time.

### Responsibilities

- Inspect all source files:
    - `Actual_Situation_Spring2023.xlsx`
    - `Linear Prediction.csv`
    - `points.csv`
    - the list of disappeared points
- Check data quality:
    - missing values
    - duplicated records
    - incorrect dates
    - inconsistent identifiers
    - negative or impossible quantities
    - fill rates outside the expected range
    - differences between litres and kilograms
- Resolve point-identifier inconsistencies. In particular:
    - the map contains 328 collection points
    - the actual data contains 300 active points
    - the disappeared-points document lists 26 points
    - points 117 and 174 are also absent from the operational data
- Produce descriptive statistics:
    - number of collections per point
    - average collected mass
    - average fill rate
    - average daily waste generation
    - differences between municipalities
    - changes over the study period
- Evaluate the supplied linear prediction.
- If necessary, develop alternative forecasting approaches:
    - historical average
    - linear regression
    - weekday-based average
    - point-specific daily generation rate
- Export one standardized demand file.

### Expected output

A file such as:

```
processed/daily_demand.csv
```

with columns:

- `point_id`
- `date`
- `predicted_daily_kg`
- `predicted_cumulative_kg`
- `capacity_kg`
- `active`

The participant should also provide:

- a data-quality report
- a list of excluded points
- plots of daily and cumulative waste generation
- forecasting error metrics
- a short explanation of all assumptions

### Independent starting point

This participant can work directly with the Excel and CSV files from the beginning.

The other participants do not need to wait for this work because:

- Role 2 can use the existing `Linear Prediction.csv`
- Role 3 can use the actual collections or synthetic demands
- Role 4 can use the existing routes and maps

When the cleaned demand file is ready, it replaces the temporary input without changing the other components.

### Interface with the team

The participant must preserve the agreed columns and point identifiers. The rest of the team should not depend on the internal forecasting code.

The only required interface is:

```python
def load_daily_demand(path):
    """
    Returns demand records with:
    point_id, date, predicted_daily_kg,
    capacity_kg and active.
    """
```

### Success criteria

- Every active point has a valid identifier.
- Every prediction has a date and a non-negative quantity.
- Units are explicitly documented.
- Excluded points are listed.
- Forecasting error is measured, not only visually discussed.
- The output can be read by the scheduling algorithm without manual editing.

---

## Role 2 — Collection Scheduling and Optimization Specialist

### Main objective

Decide which collection points should be visited on each day.

This role focuses on collection timing, not on the detailed order in which a truck visits the points.

### Responsibilities

- Formulate the inventory dynamics. For every point:

  $$q_{i,t+1} = q_{i,t} + g_{i,t} - z_{i,t}$$

  where:
    - $q_{i,t}$ is the amount of waste stored
    - $g_{i,t}$ is predicted waste generation
    - $z_{i,t}$ is collected waste
- Define the collection rules:
    - mandatory collection before predicted overflow
    - safety threshold, such as 85%, 90% or 95%
    - minimum useful collection quantity
    - maximum number of days without collection
- Implement a simple baseline:
  > Collect a point when its predicted fill rate exceeds 85%.
- Develop the improved scheduling algorithm:
    - calculate urgency
    - identify mandatory points
    - rank optional points
    - consider a rolling planning horizon
    - distribute expected workload across days
- Build a small ILP/MILP model for small instances. The exact model can be used for:
    - 10–25 points
    - 3–7 days
    - comparison with the heuristic
- Run sensitivity tests:
    - different collection thresholds
    - different forecasting levels
    - different planning horizons
    - increased or reduced waste generation
- Export a daily collection plan.

### Expected output

A file such as:

```
results/collection_schedule.csv
```

with columns:

- `date`
- `point_id`
- `predicted_kg`
- `fill_rate`
- `mandatory`
- `urgency_score`

The participant should also provide:

- the mathematical formulation
- a baseline scheduling algorithm
- an improved algorithm
- small exact ILP results
- comparisons between thresholds
- documentation of all parameters

### Independent starting point

This role does not need to wait for Role 1.

At the beginning, the participant can use:

```
Data/Linear Prediction.csv
```

It already contains daily predictions for 300 points.

The participant should also create a small synthetic test:

```
10 points × 7 days
```

When Role 1 produces cleaned or improved predictions, only the input file changes. The scheduling code should remain unchanged.

### Interface with the team

Input:

```python
daily_demand = [
    {
        "point_id": 12,
        "date": "2023-04-14",
        "predicted_daily_kg": 35.0,
        "capacity_kg": 500.0
    }
]
```

Output:

```python
daily_schedule = {
    "2023-04-14": [12, 18, 36],
    "2023-04-15": [5, 20, 41]
}
```

The routing specialist only needs this output. They do not need to know how the urgency score or ILP was calculated.

### Success criteria

- No predicted overflow, unless explicitly allowed and penalized.
- Every scheduled point has a valid reason for collection.
- Daily expected workload is feasible for the available fleet.
- The algorithm works with both the supplied prediction and cleaned predictions.
- Small instances can be compared with an exact ILP solution.
- Parameters are configurable rather than hard-coded.

---

## Role 3 — Vehicle Routing and Algorithm Validation Specialist

### Main objective

Given a set of points that must be collected on a particular day, construct feasible and short truck routes.

### Responsibilities

- Load:
    - `distances.json`
    - `durations.json`
    - `points.csv`
    - service-time data
- Implement a simple routing baseline:
    - nearest neighbour; or
    - Clarke–Wright savings algorithm
- Implement the main routing method using OR-Tools.
- Include operational constraints:
    - all routes start at depot 328
    - all routes end at depot 328
    - each scheduled point is visited exactly once
    - truck capacity is measured in kilograms
    - maximum route duration is respected
    - service time is included
    - intermediate depot returns are supported if necessary
- Add route-improvement procedures:
    - 2-opt
    - relocate
    - swap
    - insertion of nearby optional points
- Develop an independent solution validator.
- Export routes in the format required by the map tools.

### Expected output

A route file such as:

```json
[
  [328, 15, 20, 46, 328],
  [328, 84, 91, 103, 328]
]
```

The participant should also provide:

- routing code
- baseline and improved methods
- route feasibility validator
- capacity and duration tests
- route statistics
- output compatible with the existing visualization tools

### Independent starting point

This participant does not need to wait for Roles 1 or 2.

Three inputs are immediately available:

- The real points grouped by Turn number in the Excel file.
- The example routes supplied in the map folder.
- A synthetic list such as:

```python
  daily_points = [3, 8, 14, 25, 40]
```

The routing code should accept any list of points. When Role 2 produces the final daily schedule, that list simply becomes the new input.

### Interface with the team

Input:

```python
solve_routes(
    selected_points=[3, 8, 14, 25, 40],
    demands_kg={
        3: 800,
        8: 1200,
        14: 600,
        25: 900,
        40: 700
    },
    number_of_trucks=6,
    truck_capacity_kg=10000,
    depot=328
)
```

Output:

```python
[
    {
        "vehicle": 0,
        "route": [328, 3, 14, 25, 328],
        "load_kg": 2300,
        "distance_m": 18400,
        "duration_s": 7200
    }
]
```

### Success criteria

- Every route begins and ends at the depot.
- Every required point is visited exactly once.
- Vehicle capacity is never exceeded.
- Route duration is valid.
- Total distance is independently recalculated from `distances.json`.
- The output can be displayed without manual modification.
- The method is compared with at least one simple routing baseline.

---

## Role 4 — Experimental Evaluation, Visualization and Integration Specialist

### Main objective

Build the evaluation framework, compare all algorithms, generate maps and coordinate the final technical integration.

This person is not only the report writer. They own the independent experiment and verification system.

### Responsibilities

- Define the common evaluation metrics:
    - total distance
    - total duration
    - number of routes
    - number of trucks used
    - collected kilograms
    - kilograms per kilometre
    - average vehicle load
    - number of overflows
    - number of low-fill collections
    - algorithm runtime
- Reconstruct the real Spring 2023 baseline.
- Create a shared experiment runner. For example:

```python
  run_experiment(
      demand_file,
      scheduling_method,
      routing_method,
      parameters
  )
```

- Prepare the experiment matrix:

  | Experiment | Scheduling      | Routing                        |
    | ---------- | --------------- | ------------------------------ |
  | A          | Actual dates    | Actual or reconstructed routes |
  | B          | Actual dates    | Clarke–Wright                  |
  | C          | 85% threshold   | OR-Tools                       |
  | D          | Urgency method  | OR-Tools                       |
  | E          | Rolling horizon | OR-Tools plus insertion        |

- Create standard result tables and graphs.
- Use the existing offline and online map tools.
- Check that results are reproducible from a clean environment.
- Maintain the report structure and collect text from the other participants.
- Verify that every conclusion in the report is supported by an experiment.
- Prepare the final presentation.

### Expected output

The participant should produce:

- an experiment runner
- a common metrics module
- baseline results
- comparison tables
- route maps
- runtime plots
- scenario and sensitivity results
- the integrated report
- presentation slides
- reproducibility instructions

### Independent starting point

This role can begin immediately.

The participant can use:

- the existing actual Excel data for baseline calculations
- `exemple_routes.json` for testing the metrics
- the existing offline and online maps
- synthetic schedules and routes
- the two previous reports for designing the report structure

They do not need to wait for the final algorithms.

For example, they can create a mock result:

```python
mock_routes = [
    [328, 1, 5, 8, 328],
    [328, 3, 7, 9, 328]
]
```

and immediately test:

- distance calculation
- map generation
- result-table generation
- validation
- report plots

### Interface with the team

The experiment runner should treat scheduling and routing as replaceable components:

```python
schedule = scheduling_method(demand, parameters)
routes = routing_method(schedule, routing_data, parameters)
metrics = evaluate(schedule, routes, demand)
```

This makes it possible to compare methods without rewriting the evaluation code.

### Success criteria

- Every method is evaluated using the same metrics.
- Baseline and proposed algorithms are directly comparable.
- Maps are generated automatically.
- Tables are generated from result files, not copied manually.
- Experiments can be repeated using one command.
- The final report clearly distinguishes assumptions, methods and measured results.

---

# Why These Roles Can Work in Parallel

The division avoids a sequential workflow.

| Participant                | Can start with                                         | Does not need to wait for |
| -------------------------- | ------------------------------------------------------ | ------------------------- |
| Data and forecasting       | Excel and prediction CSV                               | Any algorithm             |
| Scheduling                 | Existing Linear Prediction.csv and synthetic data      | Cleaned final dataset     |
| Routing                    | Actual daily points, example routes and distance matrix | Final schedule           |
| Evaluation and integration | Actual baseline, example routes and mock results       | Final algorithms          |

Only the final integration requires completed outputs from the other participants. Development itself remains parallel.

## Replaceable inputs

Each participant initially uses a temporary but correctly formatted input:

- Role 1 later replaces the preliminary demand file.
- Role 2 later replaces the mock collection schedule.
- Role 3 later replaces the mock routes.
- Role 4 automatically recalculates the final experiments.

Therefore, the code interfaces remain stable while the data and algorithms improve.

---

# Recommended Coordination Rules

## Rule 1 — Agree on interfaces before implementation

The team should agree on:

- point identifiers
- depot identifier
- units
- date format
- schedule format
- route format
- metric definitions

This should happen during the first meeting.

## Rule 2 — Every participant creates mock input

No participant should say:

> I cannot start because I do not have the other participant's result.

They should create a small correctly formatted mock file instead.

## Rule 3 — Use kilograms as the capacity unit

Distance should be stored in metres, duration in seconds and waste in kilograms.

Conversions should happen only when displaying results.

## Rule 4 — Keep algorithms behind simple functions

For example:

```python
load_demand()
create_schedule()
solve_routes()
validate_solution()
calculate_metrics()
```

The internal implementation can change without affecting the rest of the project.

## Rule 5 — Integrate once per week

A suggested integration schedule:

1. **Checkpoint 1:** all functions run on synthetic data
2. **Checkpoint 2:** all functions run on preliminary real data
3. **Checkpoint 3:** baseline experiment is complete
4. **Checkpoint 4:** improved algorithm is complete
5. **Checkpoint 5:** final sensitivity analysis and report

## Rule 6 — Each output must have a fallback version

- If the improved forecast fails, use the supplied linear prediction.
- If the ILP is too slow, use the threshold heuristic.
- If the advanced routing fails, use Clarke–Wright or OR-Tools.
- If the online map fails, use the offline map.

This ensures that one unfinished component does not block the entire project.

---

# Short Version to Send to the Group

I suggest dividing the project into four independent workstreams:

- **Data and Demand Forecasting:** clean and analyse the input data, resolve point identifiers, evaluate the supplied prediction and produce standardized daily demand estimates.
- **Collection Scheduling and Optimization:** decide which points should be collected on each day using thresholds, urgency scores, a rolling horizon and a small ILP model.
- **Vehicle Routing and Validation:** build feasible truck routes using Clarke–Wright or OR-Tools, enforce capacity and duration constraints, and independently validate every route.
- **Experimental Evaluation and Integration:** reconstruct the real baseline, define common metrics, automate experiments, generate maps, compare methods and coordinate the final report.

All four roles can start immediately. The forecasting role uses the raw files, the scheduling role uses the existing linear prediction, the routing role uses actual or synthetic daily point lists, and the evaluation role uses example routes and mock results. We only need to agree in advance on file formats, units and function interfaces. Final outputs can then replace temporary inputs without rewriting the other components.