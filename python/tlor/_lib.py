import ctypes
import os
import sys
from ctypes import POINTER, c_char, c_char_p, c_double, c_int, c_int32, c_int64, c_size_t, c_uint8, c_void_p
from pathlib import Path

_NAMES = {"linux": "libtlor.so", "darwin": "libtlor.dylib", "win32": "libtlor.dll"}


def _find_library():
    explicit = os.environ.get("TLOR_LIBRARY")
    if explicit:
        return Path(explicit)
    name = _NAMES.get(sys.platform, "libtlor.so")
    build = Path(__file__).resolve().parents[2] / "build"
    found = sorted(build.glob(f"*/bin/**/{name}"), key=lambda p: ("release" not in str(p).lower(), str(p)))
    if not found:
        raise ImportError(f"cannot find {name} under {build}; build the library or set TLOR_LIBRARY")
    return found[0]


class InstanceDesc(ctypes.Structure):
    _fields_ = [
        ("node_count", c_int32),
        ("depot", c_int32),
        ("day_count", c_int32),
        ("distance_m", POINTER(c_double)),
        ("duration_s", POINTER(c_double)),
        ("service_s", POINTER(c_double)),
        ("capacity_kg", POINTER(c_double)),
        ("active", POINTER(c_uint8)),
        ("demand_kg", POINTER(c_double)),
    ]


class Violation(ctypes.Structure):
    _fields_ = [
        ("kind", c_int32),
        ("day", c_int32),
        ("route", c_int32),
        ("point", c_int32),
        ("message", c_char_p),
    ]


class RollingOptions(ctypes.Structure):
    _fields_ = [
        ("first_day", c_int32),
        ("last_day", c_int32),
        ("horizon", c_int32),
        ("step", c_int32),
        ("schedule_method", c_char_p),
        ("schedule_params", c_void_p),
        ("route_method", c_char_p),
        ("route_params", c_void_p),
        ("improve_method", c_char_p),
        ("improve_params", c_void_p),
        ("weights", c_void_p),
        ("evaluation_params", c_void_p),
        ("check_each_step", c_int),
    ]


class ParamSpec(ctypes.Structure):
    _fields_ = [
        ("name", c_char_p),
        ("type", c_int),
        ("default_int", c_int64),
        ("default_double", c_double),
        ("default_bool", c_int),
        ("default_string", c_char_p),
        ("has_min", c_int),
        ("min", c_double),
        ("has_max", c_int),
        ("max", c_double),
        ("description", c_char_p),
    ]


H = c_void_p
PH = POINTER(c_void_p)
PD = POINTER(c_double)
PI32 = POINTER(c_int32)
PU8 = POINTER(c_uint8)
S = c_char_p
STATUS = c_int

_SIGNATURES = {
    "tlor_version": (S, []),
    "tlor_status_name": (S, [c_int]),
    "tlor_last_error": (S, []),
    "tlor_set_error": (None, [S]),
    "tlor_instance_create": (STATUS, [POINTER(InstanceDesc), PH]),
    "tlor_instance_destroy": (None, [H]),
    "tlor_instance_set_attribute": (STATUS, [H, S, PD]),
    "tlor_instance_node_count": (c_int32, [H]),
    "tlor_instance_depot": (c_int32, [H]),
    "tlor_instance_day_count": (c_int32, [H]),
    "tlor_instance_distance_m": (PD, [H]),
    "tlor_instance_duration_s": (PD, [H]),
    "tlor_instance_service_s": (PD, [H]),
    "tlor_instance_capacity_kg": (PD, [H]),
    "tlor_instance_active": (PU8, [H]),
    "tlor_instance_demand_kg": (PD, [H]),
    "tlor_instance_attribute": (PD, [H, S]),
    "tlor_fleet_create": (STATUS, [c_int32, PD, PD, c_int32, PU8, PH]),
    "tlor_fleet_destroy": (None, [H]),
    "tlor_fleet_truck_count": (c_int32, [H]),
    "tlor_fleet_day_count": (c_int32, [H]),
    "tlor_fleet_capacity_kg": (PD, [H]),
    "tlor_fleet_max_work_s": (PD, [H]),
    "tlor_fleet_available": (PU8, [H]),
    "tlor_state_create": (STATUS, [c_int32, PD, PI32, PH]),
    "tlor_state_destroy": (None, [H]),
    "tlor_state_node_count": (c_int32, [H]),
    "tlor_state_level_kg": (PD, [H]),
    "tlor_state_last_collection": (PI32, [H]),
    "tlor_params_create": (STATUS, [PH]),
    "tlor_params_destroy": (None, [H]),
    "tlor_params_set_int": (STATUS, [H, S, c_int64]),
    "tlor_params_set_double": (STATUS, [H, S, c_double]),
    "tlor_params_set_bool": (STATUS, [H, S, c_int]),
    "tlor_params_set_string": (STATUS, [H, S, S]),
    "tlor_params_size": (c_size_t, [H]),
    "tlor_params_key": (S, [H, c_size_t]),
    "tlor_params_type": (STATUS, [H, S, POINTER(c_int)]),
    "tlor_params_get_int": (STATUS, [H, S, POINTER(c_int64)]),
    "tlor_params_get_double": (STATUS, [H, S, PD]),
    "tlor_params_get_bool": (STATUS, [H, S, POINTER(c_int)]),
    "tlor_params_get_string": (STATUS, [H, S, POINTER(c_char_p)]),
    "tlor_plan_create": (STATUS, [c_int32, c_int32, PH]),
    "tlor_plan_destroy": (None, [H]),
    "tlor_plan_add": (STATUS, [H, c_int32, c_int32]),
    "tlor_plan_remove": (STATUS, [H, c_int32, c_int32]),
    "tlor_plan_start": (c_int32, [H]),
    "tlor_plan_horizon": (c_int32, [H]),
    "tlor_plan_points": (STATUS, [H, c_int32, POINTER(PI32), POINTER(c_size_t)]),
    "tlor_plan_assign": (STATUS, [H, H]),
    "tlor_day_routes_create": (STATUS, [c_int32, PH]),
    "tlor_day_routes_destroy": (None, [H]),
    "tlor_day_routes_add": (STATUS, [H, c_int32, PI32, c_size_t]),
    "tlor_day_routes_day": (c_int32, [H]),
    "tlor_day_routes_count": (c_size_t, [H]),
    "tlor_day_routes_get": (STATUS, [H, c_size_t, PI32, POINTER(PI32), POINTER(c_size_t)]),
    "tlor_day_routes_assign": (STATUS, [H, H]),
    "tlor_solution_create": (STATUS, [H, PH]),
    "tlor_solution_destroy": (None, [H]),
    "tlor_solution_plan": (H, [H]),
    "tlor_solution_set_routes": (STATUS, [H, H]),
    "tlor_solution_routes": (H, [H, c_int32]),
    "tlor_solution_assign": (STATUS, [H, H]),
    "tlor_report_create": (STATUS, [PH]),
    "tlor_report_destroy": (None, [H]),
    "tlor_report_set_context": (STATUS, [H, S, S]),
    "tlor_report_phase_begin": (STATUS, [H, S]),
    "tlor_report_phase_end": (STATUS, [H]),
    "tlor_report_count": (STATUS, [H, S, c_int64]),
    "tlor_report_write_json": (STATUS, [H, S]),
    "tlor_instance_load": (STATUS, [S, S, S, c_int, PH, POINTER(c_char)]),
    "tlor_write_solution": (STATUS, [S, H, H, H, PD, S]),
    "tlor_write_routes_json": (STATUS, [S, H]),
    "tlor_schedule": (STATUS, [H, H, H, c_int32, c_int32, S, H, H, H, PH]),
    "tlor_route": (STATUS, [H, H, c_int32, PI32, PD, c_size_t, S, H, H, H, PH]),
    "tlor_improve": (STATUS, [H, H, H, H, S, H, S, H, H, H, PH]),
    "tlor_simulate": (STATUS, [H, H, c_int32, c_int32, H, PD, PH, PD]),
    "tlor_evaluate": (STATUS, [H, H, H, H, PD, H, PH]),
    "tlor_evaluation_destroy": (None, [H]),
    "tlor_evaluation_total": (H, [H]),
    "tlor_evaluation_start_day": (c_int32, [H]),
    "tlor_evaluation_day_count": (c_int32, [H]),
    "tlor_evaluation_day": (H, [H, c_int32]),
    "tlor_indicator_count": (c_size_t, []),
    "tlor_indicator_name": (S, [c_size_t]),
    "tlor_indicators_get": (STATUS, [H, S, PD]),
    "tlor_objective": (STATUS, [H, H, PD]),
    "tlor_check": (STATUS, [H, H, H, H, PD, PH]),
    "tlor_violations_destroy": (None, [H]),
    "tlor_violations_count": (c_size_t, [H]),
    "tlor_violations_get": (STATUS, [H, c_size_t, POINTER(Violation)]),
    "tlor_violation_kind_name": (S, [c_int32]),
    "tlor_run_rolling": (STATUS, [H, H, H, POINTER(RollingOptions), PD, H, PH]),
    "tlor_rolling_result_destroy": (None, [H]),
    "tlor_rolling_result_solution": (H, [H]),
    "tlor_rolling_result_evaluation": (H, [H]),
    "tlor_rolling_result_violations": (H, [H]),
    "tlor_rolling_result_end_state": (H, [H]),
}


def _load():
    library = ctypes.CDLL(str(_find_library()))
    for name, (restype, argtypes) in _SIGNATURES.items():
        function = getattr(library, name)
        function.restype = restype
        function.argtypes = argtypes
    return library


lib = _load()


def declare(name, restype, argtypes):
    function = getattr(lib, name)
    function.restype = restype
    function.argtypes = argtypes
    return function


def text(value):
    return None if value is None else str(value).encode("utf-8")
