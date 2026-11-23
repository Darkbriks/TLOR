from ctypes import POINTER, byref, c_double, c_int32, c_void_p
from dataclasses import dataclass

import numpy as np

from ._lib import RollingOptions, Violation as _Violation, lib, text
from .errors import InvalidArgument, check_status
from .model import (DayRoutes, Handle, Params, Plan, Solution, State, as_doubles, create, handle_of, params_handle,
                    pointer)


@dataclass(frozen=True)
class Violation:
    kind: str
    day: int
    route: int
    point: int
    message: str


def indicator_names():
    return [lib.tlor_indicator_name(i).decode() for i in range(lib.tlor_indicator_count())]


def _indicators(handle):
    values = {}
    for name in indicator_names():
        out = c_double()
        check_status(lib.tlor_indicators_get(handle, text(name), byref(out)))
        values[name] = out.value
    return values


class Evaluation(Handle):
    _destroy = "tlor_evaluation_destroy"

    def __copy__(self):
        return self

    def _load(self):
        self.start = lib.tlor_evaluation_start_day(self._handle)
        self.total = _indicators(lib.tlor_evaluation_total(self._handle))
        self.per_day = [_indicators(lib.tlor_evaluation_day(self._handle, offset))
                        for offset in range(lib.tlor_evaluation_day_count(self._handle))]
        return self

    def objective(self, weights):
        w = Params(weights)
        out = c_double()
        check_status(lib.tlor_objective(lib.tlor_evaluation_total(self._handle), w._handle, byref(out)))
        return out.value


@dataclass
class RollingResult:
    solution: Solution
    evaluation: Evaluation
    violations: list
    end_state: State


class _RollingHandle(Handle):
    _destroy = "tlor_rolling_result_destroy"


class _ViolationsHandle(Handle):
    _destroy = "tlor_violations_destroy"


def _demand(instance, demand_kg, what="demand_kg"):
    if demand_kg is None:
        return None
    return as_doubles(demand_kg, (instance.day_count, instance.node_count), what)


def _demand_pointer(array):
    return None if array is None else pointer(array, c_double)


def _violations(handle):
    result = []
    for index in range(lib.tlor_violations_count(handle)):
        v = _Violation()
        check_status(lib.tlor_violations_get(handle, index, byref(v)))
        result.append(Violation(lib.tlor_violation_kind_name(v.kind).decode(), v.day, v.route, v.point,
                                v.message.decode("utf-8", "replace")))
    return result


def _choice(value, what):
    if value is None:
        return None, None
    if isinstance(value, str):
        return value, None
    try:
        name, params = value
    except (TypeError, ValueError):
        raise TypeError(f"{what} must be a method name or a (name, params) pair") from None
    return name, params_handle(params)


def schedule(instance, fleet, state, start, horizon, method, params=None, weights=None, report=None):
    p, w = params_handle(params), params_handle(weights)
    handle = create(lib.tlor_schedule, instance._handle, fleet._handle, handle_of(state), int(start), int(horizon),
                    text(method), handle_of(p), handle_of(w), handle_of(report))
    return Plan.own(handle)


def route(instance, fleet, day, points, loads_kg, method, params=None, weights=None, report=None):
    points = np.ascontiguousarray(points, dtype=np.int32)
    loads = as_doubles(loads_kg)
    if points.shape != loads.shape or points.ndim != 1:
        raise InvalidArgument(1, f"{points.size} points but {loads.size} loads")
    p, w = params_handle(params), params_handle(weights)
    handle = create(lib.tlor_route, instance._handle, fleet._handle, int(day), pointer(points, c_int32),
                    pointer(loads, c_double), points.size, text(method), handle_of(p), handle_of(w),
                    handle_of(report))
    return DayRoutes.own(handle)


def improve(instance, fleet, state, solution, route_method, method, route_params=None, params=None, weights=None,
            report=None):
    rp, p, w = params_handle(route_params), params_handle(params), params_handle(weights)
    handle = create(lib.tlor_improve, instance._handle, fleet._handle, handle_of(state), solution._handle,
                    text(route_method), handle_of(rp), text(method), handle_of(p), handle_of(w), handle_of(report))
    return Solution.own(handle)


def simulate(instance, state, start, days, plan, demand_kg=None):
    demand = _demand(instance, demand_kg)
    levels = np.zeros((int(days), instance.node_count), dtype=np.float64)
    end = c_void_p()
    check_status(lib.tlor_simulate(instance._handle, handle_of(state), int(start), int(days), plan._handle,
                                   _demand_pointer(demand), byref(end), pointer(levels, c_double)))
    return State.own(end.value), levels


def evaluate(instance, fleet, state, solution, demand_kg=None, params=None):
    demand = _demand(instance, demand_kg)
    p = params_handle(params)
    handle = create(lib.tlor_evaluate, instance._handle, fleet._handle, handle_of(state), solution._handle,
                    _demand_pointer(demand), handle_of(p))
    return Evaluation.own(handle)._load()


def check(instance, fleet, state, solution, demand_kg=None):
    demand = _demand(instance, demand_kg)
    handle = _ViolationsHandle.own(create(lib.tlor_check, instance._handle, fleet._handle, handle_of(state),
                                          solution._handle, _demand_pointer(demand)))
    return _violations(handle._handle)


def run_rolling(instance, fleet, state=None, *, first, last, horizon, step, schedule, route, improve=None,
                weights=None, evaluation=None, check_each_step=False, realized_demand_kg=None, report=None):
    schedule_name, schedule_params = _choice(schedule, "schedule")
    route_name, route_params = _choice(route, "route")
    improve_name, improve_params = _choice(improve, "improve")
    w, e = params_handle(weights), params_handle(evaluation)
    options = RollingOptions(
        int(first), int(last), int(horizon), int(step),
        text(schedule_name), handle_of(schedule_params), text(route_name), handle_of(route_params),
        text(improve_name), handle_of(improve_params), handle_of(w), handle_of(e), int(bool(check_each_step)),
    )
    realized = _demand(instance, realized_demand_kg, "realized_demand_kg")
    owner = _RollingHandle.own(create(lib.tlor_run_rolling, instance._handle, fleet._handle, handle_of(state),
                                      byref(options), _demand_pointer(realized), handle_of(report)))
    h = owner._handle
    return RollingResult(
        solution=Solution.copy_of(lib.tlor_rolling_result_solution(h)),
        evaluation=Evaluation.borrow(lib.tlor_rolling_result_evaluation(h), owner)._load(),
        violations=_violations(lib.tlor_rolling_result_violations(h)),
        end_state=State.copy_of(lib.tlor_rolling_result_end_state(h)),
    )
