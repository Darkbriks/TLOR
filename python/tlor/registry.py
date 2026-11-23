import ctypes
from ctypes import CFUNCTYPE, POINTER, byref, c_char_p, c_double, c_int, c_int32, c_size_t, c_void_p
from dataclasses import dataclass, field

import numpy as np

from ._lib import ParamSpec as _ParamSpec, declare, lib, text
from .errors import InvalidArgument, check_status
from .model import DayRoutes, Fleet, Instance, Params, Plan, Solution, State, read
from .report import Report

_STEPS = {"schedule": 0, "route": 1, "improve": 2}
_TYPES = ["int", "double", "bool", "string"]
_CALLBACK = CFUNCTYPE(c_int, c_void_p, c_void_p, c_void_p)
_callbacks = {}

H = c_void_p
_f = {
    "count": declare("tlor_method_count", c_size_t, [c_int]),
    "name": declare("tlor_method_name", c_char_p, [c_int, c_size_t]),
    "description": declare("tlor_method_description", c_char_p, [c_int, c_char_p]),
    "param_count": declare("tlor_method_param_count", c_int, [c_int, c_char_p, POINTER(c_size_t)]),
    "param": declare("tlor_method_param", c_int, [c_int, c_char_p, c_size_t, POINTER(_ParamSpec)]),
    "unregister": declare("tlor_unregister_method", c_int, [c_int, c_char_p]),
    "register_schedule": declare("tlor_register_schedule_method", c_int,
                                 [c_char_p, c_char_p, POINTER(_ParamSpec), c_size_t, _CALLBACK, c_void_p]),
    "register_route": declare("tlor_register_route_method", c_int,
                              [c_char_p, c_char_p, POINTER(_ParamSpec), c_size_t, _CALLBACK, c_void_p]),
    "register_improve": declare("tlor_register_improve_method", c_int,
                                [c_char_p, c_char_p, POINTER(_ParamSpec), c_size_t, _CALLBACK, c_void_p]),
}
for _name in ("instance", "fleet", "state", "params", "weights", "report"):
    _f[f"schedule_{_name}"] = declare(f"tlor_schedule_request_{_name}", H, [H])
_f["schedule_start_day"] = declare("tlor_schedule_request_start_day", c_int32, [H])
_f["schedule_horizon"] = declare("tlor_schedule_request_horizon", c_int32, [H])
for _name in ("instance", "fleet", "params", "weights", "report"):
    _f[f"route_{_name}"] = declare(f"tlor_route_request_{_name}", H, [H])
_f["route_day"] = declare("tlor_route_request_day", c_int32, [H])
_f["route_points"] = declare("tlor_route_request_points", c_size_t, [H, POINTER(POINTER(c_int32))])
_f["route_loads_kg"] = declare("tlor_route_request_loads_kg", POINTER(c_double), [H])
for _name in ("instance", "fleet", "state", "solution", "params", "weights", "report", "route_params"):
    _f[f"improve_{_name}"] = declare(f"tlor_improve_request_{_name}", H, [H])
_f["improve_route_method"] = declare("tlor_improve_request_route_method", c_char_p, [H])


@dataclass(frozen=True)
class ParamSpec:
    name: str
    type: str
    default: object
    min: float = None
    max: float = None
    description: str = ""

    def __post_init__(self):
        if self.type not in _TYPES:
            raise ValueError(f"parameter type must be one of {_TYPES}, not {self.type!r}")


@dataclass(frozen=True)
class MethodInfo:
    name: str
    description: str
    params: list = field(default_factory=list)


@dataclass
class ScheduleRequest:
    instance: Instance
    fleet: Fleet
    state: State
    start: int
    horizon: int
    params: dict
    weights: dict
    report: object


@dataclass
class RouteRequest:
    instance: Instance
    fleet: Fleet
    day: int
    points: np.ndarray
    loads_kg: np.ndarray
    params: dict
    weights: dict
    report: object


@dataclass
class ImproveRequest:
    instance: Instance
    fleet: Fleet
    state: State
    solution: Solution
    route_method: str
    route_params: dict
    params: dict
    weights: dict
    report: object


def _step(step):
    if step not in _STEPS:
        raise InvalidArgument(1, f"step must be one of {list(_STEPS)}, not {step!r}")
    return _STEPS[step]


def _spec_of(raw):
    kind = _TYPES[raw.type]
    default = {
        "int": raw.default_int,
        "double": raw.default_double,
        "bool": bool(raw.default_bool),
        "string": (raw.default_string or b"").decode(),
    }[kind]
    return ParamSpec(raw.name.decode(), kind, default, raw.min if raw.has_min else None,
                     raw.max if raw.has_max else None, (raw.description or b"").decode())


def methods(step):
    code = _step(step)
    result = []
    for index in range(_f["count"](code)):
        name = _f["name"](code, index)
        count = c_size_t()
        check_status(_f["param_count"](code, name, byref(count)))
        params = []
        for i in range(count.value):
            raw = _ParamSpec()
            check_status(_f["param"](code, name, i, byref(raw)))
            params.append(_spec_of(raw))
        result.append(MethodInfo(name.decode(), (_f["description"](code, name) or b"").decode(), params))
    return result


def _raw_specs(params):
    specs = (_ParamSpec * max(len(params), 1))()
    keep = []
    for index, spec in enumerate(params):
        raw = specs[index]
        name = text(spec.name)
        description = text(spec.description)
        keep += [name, description]
        raw.name = name
        raw.type = _TYPES.index(spec.type)
        raw.description = description
        if spec.type == "int":
            raw.default_int = int(spec.default)
        elif spec.type == "double":
            raw.default_double = float(spec.default)
        elif spec.type == "bool":
            raw.default_bool = int(bool(spec.default))
        else:
            value = text(spec.default)
            keep.append(value)
            raw.default_string = value
        if spec.min is not None:
            raw.has_min, raw.min = 1, float(spec.min)
        if spec.max is not None:
            raw.has_max, raw.max = 1, float(spec.max)
    return specs, keep


def _dict(handle, owner):
    return Params.borrow(handle, owner).to_dict()


def _report(handle, owner):
    return None if not handle else Report.borrow(handle, owner)


def _borrowed(request):
    return [value for value in vars(request).values() if isinstance(value, (Instance, Fleet, State, Report))
            and value.__dict__.get("_owner") is not None]


def _schedule_request(r):
    return ScheduleRequest(
        Instance.borrow(_f["schedule_instance"](r), r), Fleet.borrow(_f["schedule_fleet"](r), r),
        State.borrow(_f["schedule_state"](r), r), _f["schedule_start_day"](r), _f["schedule_horizon"](r),
        _dict(_f["schedule_params"](r), r), _dict(_f["schedule_weights"](r), r), _report(_f["schedule_report"](r), r),
    )


def _route_request(r):
    points = POINTER(c_int32)()
    count = _f["route_points"](r, byref(points))
    return RouteRequest(
        Instance.borrow(_f["route_instance"](r), r), Fleet.borrow(_f["route_fleet"](r), r), _f["route_day"](r),
        read(points, (count,), np.int32) if count else np.empty(0, dtype=np.int32),
        read(_f["route_loads_kg"](r), (count,), np.float64) if count else np.empty(0),
        _dict(_f["route_params"](r), r), _dict(_f["route_weights"](r), r), _report(_f["route_report"](r), r),
    )


def _improve_request(r):
    return ImproveRequest(
        Instance.borrow(_f["improve_instance"](r), r), Fleet.borrow(_f["improve_fleet"](r), r),
        State.borrow(_f["improve_state"](r), r), Solution.copy_of(_f["improve_solution"](r)),
        _f["improve_route_method"](r).decode(), _dict(_f["improve_route_params"](r), r),
        _dict(_f["improve_params"](r), r), _dict(_f["improve_weights"](r), r), _report(_f["improve_report"](r), r),
    )


_STEP_KINDS = {
    "schedule": (_schedule_request, Plan, "tlor_plan_assign"),
    "route": (_route_request, DayRoutes, "tlor_day_routes_assign"),
    "improve": (_improve_request, Solution, "tlor_solution_assign"),
}


def register_method(step, name, fn, params=(), description=""):
    code = _step(step)
    build, expected, assign = _STEP_KINDS[step]

    def callback(request, out, _user_data):
        built = None
        try:
            built = build(request)
            result = fn(built)
            if not isinstance(result, expected):
                raise TypeError(f"a {step} method must return a {expected.__name__}, not {type(result).__name__}")
            check_status(getattr(lib, assign)(out, result._handle))
            return 0
        except BaseException as error:
            lib.tlor_set_error(text(f"{type(error).__name__}: {error}"))
            return 6
        finally:
            if built is not None:
                for borrowed in _borrowed(built):
                    borrowed.expire()

    wrapped = _CALLBACK(callback)
    specs, keep = _raw_specs(list(params))
    check_status(_f[f"register_{step}"](text(name), text(description), specs, len(params), wrapped, None))
    _callbacks[(code, name)] = (wrapped, keep)


def unregister_method(step, name):
    code = _step(step)
    check_status(_f["unregister"](code, text(name)))
    _callbacks.pop((code, name), None)
