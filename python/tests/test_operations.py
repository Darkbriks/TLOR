import json

import numpy as np
import pytest

import tlor
from conftest import week_solution


def test_route_nearest_neighbour(inst, fleet):
    r = tlor.route(inst, fleet, 1, [0, 1, 2, 3], [60, 20, 40, 100], "nearest_neighbour")
    assert list(r) == [(0, [5, 0, 1, 2, 5]), (0, [5, 3, 5])]


def test_schedule_threshold(inst, fleet):
    plan = tlor.schedule(inst, fleet, None, 0, 7, "threshold")
    assert plan.days() == {1: [3], 2: [0], 3: [3], 4: [0, 2, 3], 6: [3]}


def test_improve_shift_day(inst, fleet):
    plan = tlor.Plan(0, 2)
    plan.add(0, 0)
    plan.add(1, 1)
    result = tlor.improve(inst, fleet, None, tlor.Solution(plan), "nearest_neighbour", "shift_day",
                          weights={"distance_m": 1.0})
    days = result.plan.days()
    assert len(days) == 1
    assert sorted(next(iter(days.values()))) == [0, 1]


def test_simulate(inst):
    plan = tlor.Plan(0, 7)
    plan.add(2, 0)
    state, levels = tlor.simulate(inst, None, 0, 7, plan)
    assert levels.shape == (7, 6)
    assert levels[2, 0] == 90
    assert state.level_kg[3] == 350
    _, doubled = tlor.simulate(inst, None, 0, 3, plan, demand_kg=inst.demand_kg * 2)
    assert doubled[2, 0] == 180


def test_evaluate_objective(inst, fleet):
    ev = tlor.evaluate(inst, fleet, None, week_solution())
    assert ev.start == 0
    assert ev.total["distance_m"] == 250
    assert ev.total["overflow_point_days"] == 8
    assert len(ev.per_day) == 7
    assert ev.per_day[2]["collections"] == 1
    assert ev.objective({"distance_m": 1.0, "overflow_point_days": 1000.0}) == 8250
    low = tlor.evaluate(inst, fleet, None, week_solution(), params={"low_fill_threshold": 0.95})
    assert low.total["low_fill_collections"] == 1


def test_check_overflow(inst, fleet):
    violations = tlor.check(inst, fleet, None, tlor.Solution(tlor.Plan(0, 3)))
    assert len(violations) == 1
    v = violations[0]
    assert (v.kind, v.day, v.route, v.point) == ("overflow", 2, -1, 3)
    assert v.message


def test_errors(inst, fleet):
    with pytest.raises(tlor.UnknownMethod):
        tlor.route(inst, fleet, 1, [0], [10], "nope")
    with pytest.raises(tlor.InvalidArgument):
        tlor.route(inst, fleet, 1, [4], [10], "nearest_neighbour")
    with pytest.raises(tlor.InvalidParam):
        tlor.schedule(inst, fleet, None, 0, 7, "threshold", params={"threshold": 2.0})
    with pytest.raises(TypeError):
        tlor.schedule(inst, fleet, None, 0, 7, "threshold", params={"threshold": [0.5]})
    with pytest.raises(tlor.Error) as info:
        tlor.route(inst, fleet, 1, [0], [10], "nope")
    assert info.value.status == 2
    assert "nope" in info.value.message


def test_run_rolling(inst, fleet):
    result = tlor.run_rolling(inst, fleet, first=0, last=6, horizon=3, step=1,
                              schedule="threshold", route=("nearest_neighbour", {}))
    assert result.violations == []
    assert result.solution.plan.days()[4] == [0, 2, 3]
    assert result.evaluation.total["overflow_point_days"] == 0
    assert result.end_state.last_collection[3] == 6
    realized = inst.demand_kg * 1.2
    stressed = tlor.run_rolling(inst, fleet, first=0, last=6, horizon=3, step=1, schedule="threshold",
                                route="nearest_neighbour", realized_demand_kg=realized)
    assert stressed.violations
    assert {v.kind for v in stressed.violations} <= {"overflow", "capacity"}


def test_report_file(inst, fleet, tmp_path):
    path = tmp_path / "r.json"
    with tlor.Report(path, experiment="t") as rep:
        with rep.phase("mine"):
            rep.count("things", 2)
        tlor.route(inst, fleet, 1, [0], [60], "nearest_neighbour", report=rep)
    data = json.loads(path.read_text())
    assert data["schema_version"] == 1
    assert data["context"]["user"]["experiment"] == "t"
    assert data["calls"][0]["step"] == "route"
    assert data["phases"][0]["name"] == "mine"
    assert data["phases"][0]["counters"]["things"] == 2
    with pytest.raises(tlor.IOFailure):
        tlor.Report().write(tmp_path / "missing" / "r.json")
