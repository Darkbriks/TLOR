import numpy as np
import pytest

import tlor
from conftest import small_arrays


def test_instance_round_trip(inst):
    assert inst.node_count == 6
    assert inst.depot == 5
    assert inst.day_count == 7
    assert inst.distance_m[3, 5] == 450
    assert inst.duration_s[5, 3] == 40
    assert inst.demand_kg[6, 3] == 50
    assert inst.active.tolist() == [True, True, True, True, False, False]
    assert inst.attribute("x") is None
    inst.set_attribute("threshold", [0.9, np.nan, np.nan, np.nan, np.nan, np.nan])
    assert inst.attribute("threshold")[0] == pytest.approx(0.9)
    assert inst.first_date is None


def test_instance_errors():
    arrays = small_arrays()
    arrays["depot"] = 9
    with pytest.raises(tlor.InvalidArgument):
        tlor.Instance(**arrays)
    arrays = small_arrays()
    arrays["distance_m"] = arrays["distance_m"][:5, :5]
    with pytest.raises(tlor.InvalidArgument):
        tlor.Instance(**arrays)


def test_fleet_and_state(fleet, inst):
    assert fleet.truck_count == 2
    assert fleet.day_count == 7
    assert fleet.capacity_kg.tolist() == [150, 150]
    assert fleet.available[5].tolist() == [False, False]
    state = tlor.State.initial(inst)
    assert state.level_kg.tolist() == [0.0] * 6
    assert state.last_collection.tolist() == [-1] * 6
    custom = tlor.State(2, level_kg=[5, 6], last_collection=[0, -1])
    assert custom.level_kg.tolist() == [5, 6]


def test_plan_and_routes():
    plan = tlor.Plan(0, 2)
    plan.add(1, 3)
    plan.add(1, 0)
    assert plan.start == 0
    assert plan.horizon == 2
    assert plan.points(1).tolist() == [0, 3]
    assert plan.days() == {1: [0, 3]}
    plan.remove(1, 0)
    assert plan.days() == {1: [3]}
    with pytest.raises(tlor.InvalidArgument):
        plan.add(5, 0)
    routes = tlor.DayRoutes(1)
    routes.add(0, [5, 3, 5])
    assert routes.day == 1
    assert len(routes) == 1
    assert list(routes) == [(0, [5, 3, 5])]
    solution = tlor.Solution(plan)
    assert solution.routes(1) is None
    solution.set_routes(routes)
    assert list(solution.routes(1)) == [(0, [5, 3, 5])]
    assert solution.plan.days() == {1: [3]}
