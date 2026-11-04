## **Balanced optimization roles**

### **Role 1 — Demand Prediction and Collection-Policy Optimization**

Role 1 does not only prepare data. They optimize the collection policy for each individual point.

#### **Optimization question**

> At what fill level, or after how many days, should each point be scheduled for collection?

#### **Decisions**

For every point, determine:

* prediction method;  
* collection threshold;  
* safety margin;  
* maximum time between collections;  
* target collection frequency.

For example, different points may use different thresholds:

```
Fast-filling point: collect at 75%
Slow-filling point: collect at 90%
Remote point: collect at 85% when another nearby collection is expected
```

#### **Objective**

Minimize:

\\\[ \\text{number of collections} \+ P\_1\\cdot\\text{overflow risk} \+ P\_2\\cdot\\text{low-fill collections} \\\]

Role 1 can simulate each point independently:

```
for threshold in [0.70, 0.75, 0.80, 0.85, 0.90, 0.95]:
    simulate_inventory(point, threshold)
    evaluate_overflow_and_visits()
```

#### **Optimization methods**

* grid search over thresholds;  
* point-specific threshold optimization;  
* prediction-model selection;  
* simulation;  
* dynamic programming for one collection point;  
* clustering points by demand pattern, if useful.

#### **Output**

```
point_policy.csv
```

with:

```
point_id
predicted_daily_kg
optimal_threshold
maximum_days_without_collection
overflow_risk
recommended_collection_dates
```

#### **Independent starting point**

Role 1 can use `Linear Prediction.csv` immediately. No other participant needs to finish first.

---

### **Role 2 — Multi-Day Collection Scheduling Optimization**

Role 2 solves the fleet-level scheduling problem.

#### **Optimization question**

> On which day should each collection point be visited, given limited daily fleet capacity?

This differs from Role 1:

* Role 1 determines when a point becomes eligible or urgent.  
* Role 2 coordinates all points across the available trucks and days.

#### **Decisions**

* whether point $i$ is collected on day $t$;  
* how many collections are assigned to each day;  
* whether a non-urgent collection should be moved earlier or later;  
* how to balance expected kilograms across days.

Decision variable:

\\\[ y\_{it}= \\begin{cases} 1,&\\text{if point }i\\text{ is collected on day }t\\\\ 0,&\\text{otherwise} \\end{cases} \\\]

#### **Objective**

Minimize:

\\\[ P\_1\\cdot\\text{overflow} \+ P\_2\\cdot\\text{early collection} \+ P\_3\\cdot\\text{daily workload imbalance} \+ P\_4\\cdot\\text{estimated travel cost} \\\]

The travel cost can initially be approximated without waiting for Role 3\. For example:

$estimatedcos{t}_{it}=2{d}_{depot,i}$

or by a simple geographical grouping.

#### **Constraints**

* container accumulation;  
* no overflow;  
* maximum daily fleet capacity;  
* number of available trucks;  
* maximum expected work per day;  
* minimum or maximum collection frequency.

#### **Optimization methods**

* ILP/MILP with Pyomo and HiGHS;  
* rolling horizon;  
* greedy scheduling baseline;  
* workload-balancing heuristic;  
* local search that moves collections between nearby days.

#### **Output**

```
collection_schedule.csv
```

with:

```
date
point_id
predicted_kg
fill_rate
mandatory
assigned_day
```

#### **Independent starting point**

Role 2 can use:

* the supplied linear prediction;  
* a fixed threshold such as 85%;  
* synthetic truck capacities;  
* approximate travel costs.

Later, Role 1’s optimized policies can replace the temporary threshold.

---

### **Role 3 — Clustering and Vehicle Routing Optimization**

Role 3 owns the daily routing problem.

#### **Optimization question**

> Given the points selected for one day, how should they be assigned to trucks and in what order should they be visited?

#### **Decisions**

* number of routes;  
* assignment of points to trucks;  
* geographical clustering, if used;  
* visiting sequence;  
* depot returns;  
* optional insertion of nearby points.

#### **Objective**

Minimize:

\\\[ \\text{total distance} \+ P\_1\\cdot\\text{extra vehicles} \+ P\_2\\cdot\\text{overtime} \\\]

#### **Constraints**

* start and finish at depot `328`;  
* truck capacity in kilograms;  
* maximum route duration;  
* service time;  
* every required point visited exactly once;  
* number of available trucks.

#### **Optimization methods**

Role 3 should compare:

1. nearest neighbour;  
2. Clarke–Wright savings;  
3. OR-Tools CVRP;  
4. OR-Tools with 2-opt or local search;  
5. routing with and without geographical clustering.

If clustering is used, Role 3 optimizes it by balancing:

* geographical compactness;  
* predicted kilograms;  
* service time;  
* number of routes.

#### **Output**

```
routes.json
route_statistics.csv
```

#### **Independent starting point**

Role 3 can use:

* actual points grouped by `Turn number`;  
* example routes;  
* synthetic daily point lists;  
* the existing distance and duration matrices.

Role 3 does not need the final schedule to develop or test the routing algorithm.

---

### **Role 4 — Integrated Schedule-and-Route Improvement**

Role 4 should not only evaluate the work. They own the optimization connecting scheduling and routing.

#### **Optimization question**

> Can the total distance be reduced by moving some collections between neighbouring days after the first routes have been constructed?

This addresses an important weakness of a purely sequential approach.

Role 2 may produce a feasible schedule, but it may create inefficient routes. Role 4 improves the schedule using the real route cost calculated by Role 3\.

#### **Decisions**

Role 4 can test moves such as:

* move one point from Tuesday to Monday;  
* move one point from Tuesday to Wednesday;  
* exchange two points between days;  
* collect a nearby point earlier;  
* remove a low-fill point from an expensive route;  
* merge two small routes;  
* insert an optional point into an existing route.

#### **Objective**

Minimize the actual integrated cost:

\\\[ \\text{total route distance} \+ P\_1\\cdot\\text{overflow} \+ P\_2\\cdot\\text{overtime} \+ P\_3\\cdot\\text{low-fill collections} \\\]

#### **Example local-search step**

```
Current plan:
Monday: points 1, 5, 8
Tuesday: points 20, 21, 22

Candidate move:
Move point 8 from Monday to Tuesday

Accept the move if:
- point 8 will not overflow;
- Tuesday remains feasible;
- total route distance decreases.
```

#### **Optimization methods**

* local search;  
* hill climbing;  
* simulated annealing;  
* large neighbourhood search;  
* parameter grid search;  
* rolling-horizon integration;  
* route-aware insertion and removal.

#### **Additional responsibility**

Role 4 also maintains the common evaluation framework:

* actual baseline;  
* common metrics;  
* sensitivity analysis;  
* comparison tables;  
* final maps.

This is appropriate because the integrated optimizer requires the same experiment runner.

#### **Output**

```
integrated_schedule.csv
integrated_routes.json
experiment_results.csv
```

#### **Independent starting point**

Role 4 does not need to wait for Roles 2 and 3\.

They can begin with:

* the actual schedule from Excel;  
* example or synthetic routes;  
* a simple nearest-neighbour routing function;  
* a mock schedule.

Later, the final scheduling and routing functions replace the temporary versions.

---

## **Clear separation between the roles**

| Role | Optimization level | Main decision |
| ----- | ----- | ----- |
| 1 | Individual collection point | Threshold and collection policy |
| 2 | All points over multiple days | Collection date |
| 3 | Points selected for one day | Truck assignment and route order |
| 4 | Complete schedule and routes | Moving points between days to reduce actual route cost |

The roles solve different optimization problems, so work is not duplicated.

## **How they connect**

```
Role 1
Optimized point policies
        ↓
Role 2
Multi-day collection schedule
        ↓
Role 3
Daily vehicle routes
        ↓
Role 4
Route-aware schedule improvement
```

This is the final integration order, but it is not the development order. All four roles can develop in parallel using temporary inputs.

## **How to avoid waiting**

| Role | Temporary input available immediately |
| ----- | ----- |
| Role 1 | `Linear Prediction.csv` |
| Role 2 | Fixed 85% threshold and supplied predictions |
| Role 3 | Actual tours or synthetic daily point lists |
| Role 4 | Actual schedule and nearest-neighbour mock routes |

Each role should expose one agreed function:

```
# Role 1
optimize_point_policies(demand_data)

# Role 2
optimize_collection_schedule(demand_data, point_policies)

# Role 3
optimize_daily_routes(selected_points, demands, fleet)

# Role 4
improve_integrated_solution(schedule, routing_function)
```

## **Workload balance**

The optimization contribution is now reasonably balanced:

* Role 1: prediction plus per-point policy optimization;  
* Role 2: mathematical scheduling model;  
* Role 3: clustering and vehicle-routing optimization;  
* Role 4: integrated local search and experimental optimization.

Data cleaning should be shared:

* each participant checks the data relevant to their component;  
* Role 1 maintains the common cleaned file;  
* the whole team agrees on identifiers, units and exclusions;  
* cleaning is not counted as Role 1’s main technical contribution.

A concise proposal for your group would be:

> Instead of assigning one person only to data preparation, each member will own a separate optimization layer: point-level collection policies, multi-day scheduling, daily vehicle routing, and integrated schedule-route improvement. Everyone can start independently using the existing prediction, actual tours or synthetic inputs, and we will connect the components through agreed file formats.

