import copy
import pickle

import pytest

import tlor


def test_copies_of_plans_are_independent():
    plan = tlor.Plan(0, 2)
    plan.add(1, 3)
    deep = copy.deepcopy(plan)
    deep.add(0, 0)
    shallow = copy.copy(plan)
    shallow.remove(1, 3)
    assert plan.days() == {1: [3]}
    assert deep.days() == {0: [0], 1: [3]}
    assert shallow.days() == {}


def test_copies_of_routes_solutions_states_and_params():
    routes = tlor.DayRoutes(1)
    routes.add(0, [5, 3, 5])
    routes_copy = copy.deepcopy(routes)
    routes_copy.add(1, [5, 2, 5])
    assert len(routes) == 1
    plan = tlor.Plan(0, 2)
    plan.add(1, 3)
    solution = tlor.Solution(plan)
    solution.set_routes(routes)
    solution_copy = copy.deepcopy(solution)
    solution_copy.set_routes(tlor.DayRoutes(1))
    assert len(solution.routes(1)) == 1
    state = tlor.State(2, level_kg=[1, 2])
    assert copy.deepcopy(state).level_kg.tolist() == [1, 2]
    params = tlor.Params({"k": 2})
    assert copy.copy(params).to_dict() == {"k": 2}


def test_instances_and_fleets_are_shared_by_copies(inst, fleet):
    assert copy.deepcopy(inst) is inst
    assert copy.copy(fleet) is fleet


def test_pickling_and_copying_a_report_are_refused(inst):
    with pytest.raises(TypeError):
        pickle.dumps(inst)
    with pytest.raises(TypeError):
        pickle.dumps(tlor.Plan(0, 1))
    with pytest.raises(TypeError):
        copy.copy(tlor.Report())


def test_objects_received_by_a_method_expire(inst, fleet):
    kept = []

    def stash(req):
        kept.append(req)
        return tlor.DayRoutes(req.day)

    tlor.register_method("route", "py_stash", stash)
    try:
        with tlor.Report() as report:
            tlor.route(inst, fleet, 1, [0], [10], "py_stash", report=report)
    finally:
        tlor.unregister_method("route", "py_stash")
    request = kept[0]
    assert request.points.tolist() == [0]
    with pytest.raises(tlor.InvalidArgument, match="expire"):
        request.instance.node_count
    with pytest.raises(tlor.InvalidArgument):
        request.fleet.truck_count
    with pytest.raises(tlor.InvalidArgument):
        request.report.count("x")
