# Role 1 — Data Tables and Column Guide

This package contains the seven CSV tables produced for Role 1, together with a bilingual, machine-readable data dictionary.

## Package contents

- `tables/`: the seven shareable CSV tables.
- `original/`: read-only source files supplied with the project.
- `documentation/`: one bilingual English/French column guide per table.
- `COLUMN_DICTIONARY_EN_FR.csv`: one row per table/column, with type, unit, English description, French description, and usage notes.
- `MANIFEST_SHA256.csv`: file checksums for integrity verification.

The tables can be regenerated from the repository root with:

```bash
python -m pip install -r scripts/requirements-role1.txt
python scripts/run_role1_analysis.py
```

## Tables

### `active_points.csv`

Canonical point register with location, observed capacity, source-presence flags, and the final active/inactive decision.

Recommended use: All roles. Filter active = True for modelling; point 328 is the depot and must not be treated as a collection point.

### `actual_point_day.csv`

Historical collections aggregated to one row per point and collection date.

Recommended use: Validation and baseline analysis. Several source container rows on the same point-date are summed.

### `daily_demand.csv`

Standardized daily demand extracted from the provided linear forecast.

Recommended use: Demand simulation input. The forecast was supplied with the subject; it was not fitted by Role 1.

### `daily_policy_features.csv`

Daily predicted state under the selected point-specific collection threshold.

Recommended use: Main hand-off to routing/scheduling. recommended_collection is a planning signal, not a final vehicle route.

### `point_policy.csv`

Selected collection threshold for each active point and its performance in three demand scenarios.

Recommended use: Point-level policy hand-off. Use threshold as a proposed operating rule; routing may require coordinated collection days.

### `prediction_validation.csv`

Observed collection events matched with forecast volume accumulated since the previous observed collection.

Recommended use: Forecast diagnostics only; do not use directly as daily routing demand.

### `threshold_evaluation.csv`

Full audit table for every tested point-threshold combination across three demand scenarios.

Recommended use: Role 1/Role 4 sensitivity analysis and policy review. point_policy.csv contains the selected row per point.

## Shared conventions

- Dates use ISO format `YYYY-MM-DD`.
- CSV files use UTF-8, `;` as separator, and `.` as decimal mark.
- Volumes are in litres (`l`); masses are in kilograms (`kg`).
- `capacity_kg` is the canonical capacity field for optimization. It is converted with each point's own kg-per-litre ratio from the supplied forecast; litre fields are retained for traceability and fill-rate calculations.
- Fill rates and thresholds are fractions: `0.80` means 80%.
- Scenario suffixes `_80pct`, `_100pct`, and `_120pct` mean demand scaled to 80%, baseline 100%, and 120% respectively.
- Blank/`NaN` mean-fill values indicate that no collection occurred in that scenario.
- The modelling set has 300 active collection points. This is derived from the current files, not imposed as a subject requirement.
- Initial simulated inventory is assumed to be zero on 2023-04-14.
- The supplied linear forecast is treated as fixed daily demand per point; Role 1 did not train this forecast.
- Collection recommendations do not account for road distances, truck capacity, route duration, or daily fleet limits. Those constraints belong to the routing/optimization stage.

## Important interpretation

`overflow_events_*` is a conservative daily capacity-exceedance diagnostic, not proof that a physical overflow occurred. The exact within-day generation and collection time is unknown.

Use `daily_policy_features.csv` as the main interface with the routing team, `point_policy.csv` for the selected operating rule, and `threshold_evaluation.csv` when reviewing or changing that rule.
