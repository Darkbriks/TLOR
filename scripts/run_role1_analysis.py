from __future__ import annotations

import argparse
import json
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from sklearn.metrics import mean_absolute_error, mean_squared_error, r2_score


DEFAULT_SOURCE_ROOT = Path(__file__).resolve().parents[1]
OUTPUT_ROOT = DEFAULT_SOURCE_ROOT / "data"
DATA_OUTPUT = OUTPUT_ROOT / "tables"
FIGURE_OUTPUT = OUTPUT_ROOT / "figures"

DISAPPEARED_IDS = {
    25,
    52,
    53,
    56,
    57,
    87,
    134,
    145,
    189,
    192,
    208,
    215,
    226,
    229,
    232,
    234,
    256,
    257,
    259,
    260,
    261,
    265,
    268,
    275,
    276,
    327,
}

THRESHOLDS = (0.70, 0.75, 0.80, 0.85, 0.90, 0.95)
DEMAND_SCENARIOS = (0.80, 1.00, 1.20)


def prepare_directories() -> None:
    DATA_OUTPUT.mkdir(parents=True, exist_ok=True)
    FIGURE_OUTPUT.mkdir(parents=True, exist_ok=True)


def load_source_data(
    source_root: Path,
) -> tuple[pd.DataFrame, pd.DataFrame, pd.DataFrame]:
    actual = pd.read_excel(
        source_root / "data" / "original" / "Actual_Situation_Spring2023.xlsx"
    )
    actual.columns = [column.strip() for column in actual.columns]
    actual["Date"] = pd.to_datetime(actual["Date"])

    prediction = pd.read_csv(
        source_root / "data" / "original" / "Linear Prediction.csv", sep=";"
    )
    prediction.columns = [column.strip() for column in prediction.columns]
    prediction["Date"] = pd.to_datetime(prediction["Date"])

    points = pd.read_csv(
        source_root / "Maps" / "offline map" / "data" / "points.csv",
        sep=";",
    )
    return actual, prediction, points


def aggregate_actual_collections(actual: pd.DataFrame) -> pd.DataFrame:
    # Repeated point/date rows usually represent multiple containers at one site.
    # They are aggregated rather than deleted.
    grouped = actual.groupby(["Identifier", "Date"], as_index=False).agg(
        description=("Description", "first"),
        first_tour=("Turn number", "min"),
        tour_count=("Turn number", "nunique"),
        container_records=("Identifier", "size"),
        collected_l=("Collected volume", "sum"),
        observed_capacity_l=("Total volume", "sum"),
        collected_kg=("Collected Kg", "sum"),
    )
    grouped["observed_fill_rate"] = (
        grouped["collected_l"] / grouped["observed_capacity_l"]
    )
    return grouped


def infer_site_capacities(actual_point_day: pd.DataFrame) -> pd.DataFrame:
    records: list[dict[str, float | int]] = []
    for point_id, group in actual_point_day.groupby("Identifier"):
        capacities = group["observed_capacity_l"]
        mode_values = capacities.mode()
        typical_capacity = float(mode_values.max())
        records.append(
            {
                "point_id": int(point_id),
                "capacity_l": typical_capacity,
                "minimum_observed_capacity_l": float(capacities.min()),
                "maximum_observed_capacity_l": float(capacities.max()),
                "capacity_variant_count": int(capacities.nunique()),
                "capacity_observation_count": int(len(capacities)),
            }
        )
    return pd.DataFrame(records).sort_values("point_id").reset_index(drop=True)


def build_active_points(
    actual: pd.DataFrame,
    prediction: pd.DataFrame,
    points: pd.DataFrame,
    capacities: pd.DataFrame,
) -> pd.DataFrame:
    actual_ids = set(actual["Identifier"].astype(int))
    prediction_ids = set(prediction["Identifier"].astype(int))
    active_ids = actual_ids & prediction_ids

    result = points.rename(columns={"index": "point_id"}).copy()
    result["point_id"] = result["point_id"].astype(int)
    result = result.merge(capacities, on="point_id", how="left")
    density = prediction[["Identifier", "Daily (Kg)", "Daily (L)"]].copy()
    density["kg_per_l"] = density["Daily (Kg)"] / density["Daily (L)"]
    density = (
        density.groupby("Identifier", as_index=False)["kg_per_l"]
        .median()
        .rename(columns={"Identifier": "point_id"})
    )
    result = result.merge(density, on="point_id", how="left")
    result["capacity_kg"] = result["capacity_l"] * result["kg_per_l"]
    result = result.drop(columns="kg_per_l")
    result["is_depot"] = result["point_id"].eq(328)
    result["in_actual"] = result["point_id"].isin(actual_ids)
    result["in_prediction"] = result["point_id"].isin(prediction_ids)
    result["listed_disappeared"] = result["point_id"].isin(DISAPPEARED_IDS)
    result["active"] = result["point_id"].isin(active_ids) & ~result["is_depot"]

    def reason(row: pd.Series) -> str:
        if row["is_depot"]:
            return "depot"
        if row["active"]:
            return "present_in_actual_and_prediction"
        if row["listed_disappeared"]:
            return "listed_as_disappeared"
        if not row["in_actual"] and not row["in_prediction"]:
            return "absent_from_actual_and_prediction"
        if not row["in_actual"]:
            return "absent_from_actual"
        return "absent_from_prediction"

    result["status_reason"] = result.apply(reason, axis=1)
    return result


def build_daily_demand(
    prediction: pd.DataFrame, active_points: pd.DataFrame
) -> pd.DataFrame:
    active_ids = set(active_points.loc[active_points["active"], "point_id"])
    demand = prediction[prediction["Identifier"].isin(active_ids)].copy()
    demand = demand.rename(
        columns={
            "Identifier": "point_id",
            "Date": "date",
            "Daily (Kg)": "daily_kg",
            "Cumulative (Kg)": "source_cumulative_kg",
            "Daily (L)": "daily_l",
            "Cumulative (L)": "source_cumulative_l",
        }
    )
    demand = demand.merge(
        active_points[["point_id", "capacity_l", "capacity_kg"]],
        on="point_id",
        how="left",
    )
    return demand.sort_values(["point_id", "date"]).reset_index(drop=True)


def simulate_threshold(
    point_demand: pd.DataFrame, threshold: float, scale: float
) -> dict[str, float | int]:
    capacity_l = float(point_demand["capacity_l"].iloc[0])
    inventory_l = 0.0
    inventory_kg = 0.0
    fills: list[float] = []
    collections = 0
    overflow_events = 0
    low_fill_events = 0

    for row in point_demand.itertuples(index=False):
        inventory_l += float(row.daily_l) * scale
        inventory_kg += float(row.daily_kg) * scale
        fill_rate = inventory_l / capacity_l
        if fill_rate >= threshold:
            collections += 1
            fills.append(fill_rate)
            overflow_events += int(fill_rate > 1.0)
            low_fill_events += int(fill_rate < 0.5)
            inventory_l = 0.0
            inventory_kg = 0.0

    return {
        "collections": collections,
        "overflow_events": overflow_events,
        "low_fill_events": low_fill_events,
        "mean_fill_at_collection": float(np.mean(fills)) if fills else np.nan,
        "ending_inventory_l": inventory_l,
        "ending_inventory_kg": inventory_kg,
    }


def evaluate_thresholds(daily_demand: pd.DataFrame) -> pd.DataFrame:
    records: list[dict[str, float | int]] = []
    for point_id, group in daily_demand.groupby("point_id"):
        for threshold in THRESHOLDS:
            record: dict[str, float | int] = {
                "point_id": int(point_id),
                "threshold": threshold,
            }
            for scale in DEMAND_SCENARIOS:
                result = simulate_threshold(group, threshold, scale)
                suffix = f"{int(scale * 100)}pct"
                for key, value in result.items():
                    record[f"{key}_{suffix}"] = value
            records.append(record)
    return pd.DataFrame(records)


def select_robust_policies(threshold_results: pd.DataFrame) -> pd.DataFrame:
    result = threshold_results.copy()
    overflow_columns = [
        f"overflow_events_{int(scale * 100)}pct" for scale in DEMAND_SCENARIOS
    ]
    result["total_scenario_overflows"] = result[overflow_columns].sum(axis=1)
    result["negative_base_fill"] = -result["mean_fill_at_collection_100pct"].fillna(0)

    # Lexicographic selection:
    # 1) minimize overflow across all scenarios;
    # 2) minimize base-case collections;
    # 3) maximize the mean fill level at collection.
    policies = (
        result.sort_values(
            [
                "point_id",
                "total_scenario_overflows",
                "collections_100pct",
                "negative_base_fill",
                "threshold",
            ]
        )
        .groupby("point_id", as_index=False)
        .first()
    )
    policies = policies.drop(columns="negative_base_fill")
    return policies.sort_values("point_id").reset_index(drop=True)


def build_daily_policy_features(
    daily_demand: pd.DataFrame, policies: pd.DataFrame
) -> pd.DataFrame:
    thresholds = policies.set_index("point_id")["threshold"].to_dict()
    records: list[dict[str, float | int | bool | pd.Timestamp]] = []

    for point_id, group in daily_demand.groupby("point_id"):
        capacity_l = float(group["capacity_l"].iloc[0])
        capacity_kg = float(group["capacity_kg"].iloc[0])
        threshold = float(thresholds[point_id])
        inventory_l = 0.0
        inventory_kg = 0.0

        for row in group.sort_values("date").itertuples(index=False):
            inventory_l += float(row.daily_l)
            inventory_kg += float(row.daily_kg)
            fill_rate = inventory_l / capacity_l
            daily_l = float(row.daily_l)
            remaining_l = max(capacity_l - inventory_l, 0.0)
            days_until_capacity = (
                int(np.floor(remaining_l / daily_l)) if daily_l > 0 else 9999
            )
            recommended = fill_rate >= threshold
            records.append(
                {
                    "point_id": int(point_id),
                    "date": row.date,
                    "daily_l": daily_l,
                    "daily_kg": float(row.daily_kg),
                    "capacity_l": capacity_l,
                    "capacity_kg": capacity_kg,
                    "selected_threshold": threshold,
                    "predicted_inventory_l": inventory_l,
                    "predicted_inventory_kg": inventory_kg,
                    "predicted_fill_rate": fill_rate,
                    "estimated_days_until_capacity": days_until_capacity,
                    "recommended_collection": recommended,
                }
            )
            if recommended:
                inventory_l = 0.0
                inventory_kg = 0.0

    return pd.DataFrame(records)


def validate_prediction_against_actual(
    actual_point_day: pd.DataFrame, daily_demand: pd.DataFrame
) -> tuple[pd.DataFrame, dict[str, float]]:
    demand_lookup = daily_demand.set_index(["point_id", "date"])[
        ["daily_kg", "daily_l"]
    ]
    start_date = daily_demand["date"].min()
    records: list[dict[str, float | int | pd.Timestamp]] = []

    for point_id, group in actual_point_day.groupby("Identifier"):
        previous_date = start_date - pd.Timedelta(days=1)
        for row in group.sort_values("Date").itertuples(index=False):
            date_range = pd.date_range(previous_date + pd.Timedelta(days=1), row.Date)
            predicted_kg = 0.0
            predicted_l = 0.0
            for date in date_range:
                try:
                    demand_row = demand_lookup.loc[(int(point_id), date)]
                    predicted_kg += float(demand_row["daily_kg"])
                    predicted_l += float(demand_row["daily_l"])
                except KeyError:
                    continue
            records.append(
                {
                    "point_id": int(point_id),
                    "date": row.Date,
                    "actual_collected_kg": float(row.collected_kg),
                    "predicted_accumulated_kg": predicted_kg,
                    "actual_collected_l": float(row.collected_l),
                    "predicted_accumulated_l": predicted_l,
                }
            )
            previous_date = row.Date

    comparison = pd.DataFrame(records)
    metrics: dict[str, float] = {}
    for unit in ("kg", "l"):
        actual_values = comparison[f"actual_collected_{unit}"]
        predicted_values = comparison[f"predicted_accumulated_{unit}"]
        metrics[f"mae_{unit}"] = float(
            mean_absolute_error(actual_values, predicted_values)
        )
        metrics[f"rmse_{unit}"] = float(
            mean_squared_error(actual_values, predicted_values) ** 0.5
        )
        metrics[f"r2_{unit}"] = float(r2_score(actual_values, predicted_values))
        metrics[f"wape_{unit}"] = float(
            (actual_values - predicted_values).abs().sum() / actual_values.sum()
        )
        metrics[f"total_bias_{unit}"] = float(
            (predicted_values.sum() - actual_values.sum()) / actual_values.sum()
        )
        metrics[f"actual_total_{unit}"] = float(actual_values.sum())
        metrics[f"predicted_total_{unit}"] = float(predicted_values.sum())
    return comparison, metrics


def create_figures(
    actual_point_day: pd.DataFrame,
    daily_demand: pd.DataFrame,
    threshold_results: pd.DataFrame,
    policies: pd.DataFrame,
    prediction_comparison: pd.DataFrame,
) -> None:
    plt.style.use("seaborn-v0_8-whitegrid")

    fig, ax = plt.subplots(figsize=(8, 4.8))
    ax.hist(actual_point_day["observed_fill_rate"], bins=25, color="#2563eb")
    ax.axvline(0.5, color="#dc2626", linestyle="--", label="50% fill")
    ax.set(
        title="Observed fill rate at collection",
        xlabel="Aggregated point fill rate",
        ylabel="Point-day collection events",
    )
    ax.legend()
    fig.tight_layout()
    fig.savefig(FIGURE_OUTPUT / "actual_fill_distribution.png", dpi=180)
    plt.close(fig)

    fixed_summary = (
        threshold_results.groupby("threshold", as_index=False)
        .agg(
            collections=("collections_100pct", "sum"),
            overflow_80pct=("overflow_events_80pct", "sum"),
            overflow_100pct=("overflow_events_100pct", "sum"),
            overflow_120pct=("overflow_events_120pct", "sum"),
        )
        .sort_values("threshold")
    )
    fig, ax1 = plt.subplots(figsize=(8, 4.8))
    ax1.plot(
        fixed_summary["threshold"],
        fixed_summary["collections"],
        marker="o",
        color="#2563eb",
        label="Base-case collections",
    )
    ax1.set_xlabel("Uniform collection threshold")
    ax1.set_ylabel("Collections", color="#2563eb")
    ax2 = ax1.twinx()
    for column, label, color in (
        ("overflow_80pct", "Overflow, demand -20%", "#16a34a"),
        ("overflow_100pct", "Overflow, base demand", "#d97706"),
        ("overflow_120pct", "Overflow, demand +20%", "#dc2626"),
    ):
        ax2.plot(
            fixed_summary["threshold"],
            fixed_summary[column],
            marker="s",
            label=label,
            color=color,
        )
    ax2.set_ylabel("Overflow events")
    handles1, labels1 = ax1.get_legend_handles_labels()
    handles2, labels2 = ax2.get_legend_handles_labels()
    ax1.legend(handles1 + handles2, labels1 + labels2, loc="upper center")
    ax1.set_title("Uniform threshold trade-off")
    fig.tight_layout()
    fig.savefig(FIGURE_OUTPUT / "threshold_tradeoff.png", dpi=180)
    plt.close(fig)

    counts = policies["threshold"].value_counts().sort_index()
    fig, ax = plt.subplots(figsize=(8, 4.8))
    ax.bar([f"{value:.0%}" for value in counts.index], counts.values, color="#7c3aed")
    ax.set(
        title="Selected robust thresholds by point",
        xlabel="Selected threshold",
        ylabel="Number of points",
    )
    fig.tight_layout()
    fig.savefig(FIGURE_OUTPUT / "selected_thresholds.png", dpi=180)
    plt.close(fig)

    fig, ax = plt.subplots(figsize=(6, 6))
    x = prediction_comparison["actual_collected_kg"]
    y = prediction_comparison["predicted_accumulated_kg"]
    ax.scatter(x, y, s=12, alpha=0.35, color="#0f766e")
    upper = max(float(x.max()), float(y.max()))
    ax.plot([0, upper], [0, upper], linestyle="--", color="#dc2626")
    ax.set(
        title="Predicted versus actual collected mass",
        xlabel="Actual collected kg",
        ylabel="Predicted accumulated kg",
    )
    fig.tight_layout()
    fig.savefig(FIGURE_OUTPUT / "prediction_validation_kg.png", dpi=180)
    plt.close(fig)

    daily_total = daily_demand.groupby("date", as_index=False)["daily_kg"].sum()
    fig, ax = plt.subplots(figsize=(9, 4.5))
    ax.plot(daily_total["date"], daily_total["daily_kg"], color="#2563eb")
    ax.set(
        title="Predicted total daily waste generation",
        xlabel="Date",
        ylabel="Predicted kg per day",
    )
    fig.autofmt_xdate()
    fig.tight_layout()
    fig.savefig(FIGURE_OUTPUT / "daily_predicted_waste.png", dpi=180)
    plt.close(fig)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run the Role 1 demand-policy analysis.")
    parser.add_argument(
        "--source-root",
        type=Path,
        default=DEFAULT_SOURCE_ROOT,
        help="Path to the original Subject1 folder.",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    prepare_directories()
    actual, prediction, points = load_source_data(args.source_root)
    actual_point_day = aggregate_actual_collections(actual)
    capacities = infer_site_capacities(actual_point_day)
    active_points = build_active_points(actual, prediction, points, capacities)
    daily_demand = build_daily_demand(prediction, active_points)
    threshold_results = evaluate_thresholds(daily_demand)
    policies = select_robust_policies(threshold_results)
    daily_policy_features = build_daily_policy_features(daily_demand, policies)
    prediction_comparison, prediction_metrics = validate_prediction_against_actual(
        actual_point_day, daily_demand
    )

    actual_point_day.rename(
        columns={"Identifier": "point_id", "Date": "date"}
    ).to_csv(DATA_OUTPUT / "actual_point_day.csv", index=False, sep=";")
    active_points.to_csv(DATA_OUTPUT / "active_points.csv", index=False, sep=";")
    daily_demand.to_csv(DATA_OUTPUT / "daily_demand.csv", index=False, sep=";")
    threshold_results.to_csv(
        DATA_OUTPUT / "threshold_evaluation.csv", index=False, sep=";"
    )
    policies.to_csv(DATA_OUTPUT / "point_policy.csv", index=False, sep=";")
    daily_policy_features.to_csv(
        DATA_OUTPUT / "daily_policy_features.csv", index=False, sep=";"
    )
    prediction_comparison.to_csv(
        DATA_OUTPUT / "prediction_validation.csv", index=False, sep=";"
    )

    rate_difference = (
        actual["Rate"] * actual["Total volume"] - actual["Collected volume"]
    ).abs()
    missing_map_ids = sorted(set(range(328)) - set(actual["Identifier"].astype(int)))
    policy_threshold_counts = {
        f"{threshold:.2f}": int(count)
        for threshold, count in policies["threshold"].value_counts().sort_index().items()
    }
    fixed_threshold_summary = (
        threshold_results.groupby("threshold")
        .agg(
            collections_100pct=("collections_100pct", "sum"),
            overflow_80pct=("overflow_events_80pct", "sum"),
            overflow_100pct=("overflow_events_100pct", "sum"),
            overflow_120pct=("overflow_events_120pct", "sum"),
            mean_fill_100pct=("mean_fill_at_collection_100pct", "mean"),
        )
        .reset_index()
        .to_dict(orient="records")
    )
    summary = {
        "source": {
            "actual_rows": int(len(actual)),
            "actual_point_day_events": int(len(actual_point_day)),
            "unique_tours": int(actual["Turn number"].nunique()),
            "actual_service_dates": int(actual["Date"].nunique()),
            "period_start": str(actual["Date"].min().date()),
            "period_end": str(actual["Date"].max().date()),
            "prediction_rows": int(len(prediction)),
            "prediction_dates": int(prediction["Date"].nunique()),
            "active_points": int(active_points["active"].sum()),
        },
        "quality": {
            "full_duplicate_rows_retained_as_possible_containers": int(
                actual.duplicated().sum()
            ),
            "repeated_point_date_rows": int(
                actual.duplicated(["Identifier", "Date"]).sum()
            ),
            "rate_volume_inconsistencies_over_1_litre": int(
                (rate_difference > 1).sum()
            ),
            "maximum_rate_volume_difference_l": float(rate_difference.max()),
            "points_with_variable_aggregated_capacity": int(
                (capacities["capacity_variant_count"] > 1).sum()
            ),
            "missing_map_point_ids": missing_map_ids,
            "missing_but_not_in_rtf": sorted(set(missing_map_ids) - DISAPPEARED_IDS),
            "rtf_ids": sorted(DISAPPEARED_IDS),
            "actual_mean_aggregated_fill": float(
                actual_point_day["observed_fill_rate"].mean()
            ),
            "actual_point_day_events_below_50pct_fill": int(
                (actual_point_day["observed_fill_rate"] < 0.5).sum()
            ),
        },
        "prediction_validation": prediction_metrics,
        "policy": {
            "method": "Lexicographic robust threshold selection across 80%, 100%, and 120% demand scenarios",
            "threshold_counts": policy_threshold_counts,
            "base_case_collections": int(policies["collections_100pct"].sum()),
            "overflow_80pct": int(policies["overflow_events_80pct"].sum()),
            "overflow_100pct": int(policies["overflow_events_100pct"].sum()),
            "overflow_120pct": int(policies["overflow_events_120pct"].sum()),
            "mean_fill_at_collection_100pct": float(
                policies["mean_fill_at_collection_100pct"].mean()
            ),
            "fixed_threshold_summary": fixed_threshold_summary,
        },
    }
    (DATA_OUTPUT / "summary.json").write_text(
        json.dumps(summary, indent=2), encoding="utf-8"
    )

    create_figures(
        actual_point_day,
        daily_demand,
        threshold_results,
        policies,
        prediction_comparison,
    )
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
