import numpy as np
import pytest

import tlor


def one_per_point(req):
    r = tlor.DayRoutes(req.day)
    for p in req.points:
        r.add(0, [req.instance.depot, int(p), req.instance.depot])
    return r


def test_builtin_listing():
    schedule = {m.name: m for m in tlor.methods("schedule")}
    route = {m.name: m for m in tlor.methods("route")}
    improve = {m.name: m for m in tlor.methods("improve")}
    assert [p.name for p in schedule["threshold"].params] == ["threshold", "use_point_threshold"]
    assert schedule["threshold"].params[0].type == "double"
    assert schedule["threshold"].params[0].default == pytest.approx(0.85)
    assert schedule["threshold"].params[0].max == 1.0
    assert route["nearest_neighbour"].params[0].default == "distance"
    assert route["nearest_neighbour"].description
    assert [p.name for p in improve["shift_day"].params] == ["max_iterations", "max_shift", "seed"]
    assert improve["shift_day"].params[2].min == 0
    with pytest.raises(tlor.InvalidArgument):
        tlor.methods("routing")


def test_python_route_method(inst, fleet):
    tlor.register_method("route", "py_one", one_per_point, params=[tlor.ParamSpec("k", "int", 2)])
    try:
        assert "py_one" in [m.name for m in tlor.methods("route")]
        assert len(tlor.route(inst, fleet, 1, [0, 1], [10, 10], "py_one")) == 2
    finally:
        tlor.unregister_method("route", "py_one")
    assert "py_one" not in [m.name for m in tlor.methods("route")]


def test_params_reach_python(inst, fleet):
    seen = {}

    def spy(req):
        seen.update(req.params)
        seen["loads"] = req.loads_kg.tolist()
        seen["weights"] = dict(req.weights)
        seen["capacity"] = req.fleet.capacity_kg.tolist()
        return tlor.DayRoutes(req.day)

    tlor.register_method("route", "py_spy", spy, params=[tlor.ParamSpec("k", "int", 2, min=0, max=10)])
    try:
        tlor.route(inst, fleet, 1, [0], [12.5], "py_spy", weights={"distance_m": 1.0})
        assert seen["k"] == 2
        assert seen["loads"] == [12.5]
        assert seen["weights"] == {"distance_m": 1.0}
        assert seen["capacity"] == [150, 150]
        with pytest.raises(tlor.InvalidParam):
            tlor.route(inst, fleet, 1, [0], [12.5], "py_spy", params={"k": 11})
    finally:
        tlor.unregister_method("route", "py_spy")


def test_python_exception_becomes_callback_error(inst, fleet):
    def broken(req):
        raise ValueError("bad input")

    tlor.register_method("route", "py_broken", broken)
    try:
        with pytest.raises(tlor.CallbackError, match="bad input"):
            tlor.route(inst, fleet, 1, [0], [10], "py_broken")
        assert len(tlor.route(inst, fleet, 1, [0], [10], "nearest_neighbour")) == 1
    finally:
        tlor.unregister_method("route", "py_broken")


def test_wrong_return_type_is_a_callback_error(inst, fleet):
    tlor.register_method("route", "py_wrong", lambda req: [(0, [5, 0, 5])])
    try:
        with pytest.raises(tlor.CallbackError, match="DayRoutes"):
            tlor.route(inst, fleet, 1, [0], [10], "py_wrong")
    finally:
        tlor.unregister_method("route", "py_wrong")


def test_python_schedule_in_rolling(inst, fleet):
    def point_three(req):
        plan = tlor.Plan(req.start, req.horizon)
        for day in range(req.start, req.start + req.horizon):
            plan.add(day, 3)
        assert req.state.level_kg.shape == (6,)
        return plan

    tlor.register_method("schedule", "py_three", point_three)
    try:
        result = tlor.run_rolling(inst, fleet, first=0, last=2, horizon=3, step=1, schedule="py_three",
                                  route="nearest_neighbour")
        assert result.solution.plan.days() == {0: [3], 1: [3], 2: [3]}
    finally:
        tlor.unregister_method("schedule", "py_three")


def test_python_route_inside_shift_day(inst, fleet):
    calls = []

    def counted(req):
        calls.append(req.day)
        return one_per_point(req)

    tlor.register_method("route", "py_counted", counted)
    try:
        plan = tlor.Plan(0, 2)
        plan.add(0, 0)
        plan.add(1, 1)
        result = tlor.improve(inst, fleet, None, tlor.Solution(plan), "py_counted", "shift_day",
                              weights={"distance_m": 1.0})
        assert isinstance(result, tlor.Solution)
        assert len(calls) >= 2
    finally:
        tlor.unregister_method("route", "py_counted")


def test_python_improve_method(inst, fleet):
    def drop_day_one(req):
        assert req.route_method == "nearest_neighbour"
        plan = req.solution.plan
        for p in plan.points(1):
            plan.remove(1, int(p))
        return tlor.Solution(plan)

    tlor.register_method("improve", "py_drop", drop_day_one)
    try:
        plan = tlor.Plan(0, 2)
        plan.add(0, 0)
        plan.add(1, 1)
        result = tlor.improve(inst, fleet, None, tlor.Solution(plan), "nearest_neighbour", "py_drop")
        assert result.plan.days() == {0: [0]}
    finally:
        tlor.unregister_method("improve", "py_drop")


def test_registration_errors():
    tlor.register_method("route", "py_dup", one_per_point)
    try:
        with pytest.raises(tlor.InvalidArgument):
            tlor.register_method("route", "py_dup", one_per_point)
    finally:
        tlor.unregister_method("route", "py_dup")
    with pytest.raises(tlor.InvalidArgument):
        tlor.unregister_method("route", "nearest_neighbour")
    with pytest.raises(tlor.UnknownMethod):
        tlor.unregister_method("route", "py_absent")
    with pytest.raises(ValueError):
        tlor.ParamSpec("x", "float", 1.0)
