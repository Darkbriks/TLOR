import ctypes
import numbers
from ctypes import POINTER, byref, c_char_p, c_double, c_int, c_int32, c_int64, c_size_t, c_uint8, c_void_p

import numpy as np

from ._lib import InstanceDesc, lib, text
from .errors import InvalidArgument, check_status


def as_doubles(values, shape=None, what="array"):
    array = np.ascontiguousarray(values, dtype=np.float64)
    if shape is not None and array.shape != shape:
        raise InvalidArgument(1, f"{what} has shape {array.shape}, expected {shape}")
    return array


def pointer(array, ctype):
    return array.ctypes.data_as(POINTER(ctype))


def read(ptr, shape, dtype):
    if not ptr:
        return None
    count = int(np.prod(shape))
    if count == 0:
        return np.empty(shape, dtype=dtype)
    return np.ctypeslib.as_array(ptr, shape=(count,)).reshape(shape).astype(dtype, copy=True)


class Handle:
    _destroy = None

    def __init__(self, handle):
        self._handle = handle
        self._owner = None

    @classmethod
    def own(cls, handle):
        obj = cls.__new__(cls)
        obj._handle = handle
        obj._owner = None
        return obj

    @classmethod
    def borrow(cls, handle, owner):
        obj = cls.__new__(cls)
        obj._handle = handle
        obj._owner = owner
        return obj

    def __del__(self):
        handle = getattr(self, "_handle", None)
        if handle and getattr(self, "_owner", True) is None and self._destroy:
            try:
                getattr(lib, self._destroy)(handle)
            except Exception:
                pass
            self._handle = None


def create(function, *args):
    out = c_void_p()
    check_status(function(*args, byref(out)))
    return out.value


class Params(Handle):
    _destroy = "tlor_params_destroy"

    def __init__(self, values=None):
        super().__init__(create(lib.tlor_params_create))
        for name, value in (values or {}).items():
            self.set(name, value)

    def set(self, name, value):
        key = text(name)
        if isinstance(value, (bool, np.bool_)):
            check_status(lib.tlor_params_set_bool(self._handle, key, int(bool(value))))
        elif isinstance(value, numbers.Integral):
            check_status(lib.tlor_params_set_int(self._handle, key, int(value)))
        elif isinstance(value, numbers.Real):
            check_status(lib.tlor_params_set_double(self._handle, key, float(value)))
        elif isinstance(value, str):
            check_status(lib.tlor_params_set_string(self._handle, key, text(value)))
        else:
            raise TypeError(f"parameter {name!r} has unsupported type {type(value).__name__}")

    def to_dict(self):
        result = {}
        for index in range(lib.tlor_params_size(self._handle)):
            name = lib.tlor_params_key(self._handle, index)
            kind = c_int()
            check_status(lib.tlor_params_type(self._handle, name, byref(kind)))
            if kind.value == 0:
                out = c_int64()
                check_status(lib.tlor_params_get_int(self._handle, name, byref(out)))
            elif kind.value == 1:
                out = c_double()
                check_status(lib.tlor_params_get_double(self._handle, name, byref(out)))
            elif kind.value == 2:
                out = c_int()
                check_status(lib.tlor_params_get_bool(self._handle, name, byref(out)))
                result[name.decode()] = bool(out.value)
                continue
            else:
                out = c_char_p()
                check_status(lib.tlor_params_get_string(self._handle, name, byref(out)))
                result[name.decode()] = out.value.decode("utf-8")
                continue
            result[name.decode()] = out.value
        return result


def params_handle(values):
    return None if values is None else Params(values)


def handle_of(obj):
    return None if obj is None else obj._handle


class Instance(Handle):
    _destroy = "tlor_instance_destroy"
    first_date = None

    def __init__(self, distance_m, duration_s, service_s, capacity_kg, active, demand_kg, depot):
        distance = as_doubles(distance_m)
        if distance.ndim != 2 or distance.shape[0] != distance.shape[1]:
            raise InvalidArgument(1, f"distance_m must be a square matrix, not {distance.shape}")
        n = distance.shape[0]
        duration = as_doubles(duration_s, (n, n), "duration_s")
        service = as_doubles(service_s, (n,), "service_s")
        capacity = as_doubles(capacity_kg, (n,), "capacity_kg")
        flags = np.ascontiguousarray(active, dtype=np.uint8)
        if flags.shape != (n,):
            raise InvalidArgument(1, f"active has shape {flags.shape}, expected {(n,)}")
        demand = as_doubles(demand_kg)
        if demand.ndim != 2 or demand.shape[1] != n:
            raise InvalidArgument(1, f"demand_kg has shape {demand.shape}, expected (days, {n})")
        desc = InstanceDesc(
            n, int(depot), demand.shape[0],
            pointer(distance, c_double), pointer(duration, c_double), pointer(service, c_double),
            pointer(capacity, c_double), pointer(flags, c_uint8), pointer(demand, c_double),
        )
        super().__init__(create(lib.tlor_instance_create, byref(desc)))

    @property
    def node_count(self):
        return lib.tlor_instance_node_count(self._handle)

    @property
    def depot(self):
        return lib.tlor_instance_depot(self._handle)

    @property
    def day_count(self):
        return lib.tlor_instance_day_count(self._handle)

    @property
    def distance_m(self):
        n = self.node_count
        return read(lib.tlor_instance_distance_m(self._handle), (n, n), np.float64)

    @property
    def duration_s(self):
        n = self.node_count
        return read(lib.tlor_instance_duration_s(self._handle), (n, n), np.float64)

    @property
    def service_s(self):
        return read(lib.tlor_instance_service_s(self._handle), (self.node_count,), np.float64)

    @property
    def capacity_kg(self):
        return read(lib.tlor_instance_capacity_kg(self._handle), (self.node_count,), np.float64)

    @property
    def active(self):
        return read(lib.tlor_instance_active(self._handle), (self.node_count,), bool)

    @property
    def demand_kg(self):
        return read(lib.tlor_instance_demand_kg(self._handle), (self.day_count, self.node_count), np.float64)

    def set_attribute(self, name, values):
        array = as_doubles(values, (self.node_count,), name)
        check_status(lib.tlor_instance_set_attribute(self._handle, text(name), pointer(array, c_double)))

    def attribute(self, name):
        return read(lib.tlor_instance_attribute(self._handle, text(name)), (self.node_count,), np.float64)


class Fleet(Handle):
    _destroy = "tlor_fleet_destroy"

    def __init__(self, capacity_kg, max_work_s, day_count, available=None):
        capacity = as_doubles(capacity_kg)
        trucks = capacity.shape[0]
        work = as_doubles(max_work_s, (trucks,), "max_work_s")
        flags = None
        if available is not None:
            flags = np.ascontiguousarray(available, dtype=np.uint8)
            if flags.shape != (day_count, trucks):
                raise InvalidArgument(1, f"available has shape {flags.shape}, expected {(day_count, trucks)}")
        super().__init__(create(
            lib.tlor_fleet_create, trucks, pointer(capacity, c_double), pointer(work, c_double), int(day_count),
            None if flags is None else pointer(flags, c_uint8),
        ))

    @property
    def truck_count(self):
        return lib.tlor_fleet_truck_count(self._handle)

    @property
    def day_count(self):
        return lib.tlor_fleet_day_count(self._handle)

    @property
    def capacity_kg(self):
        return read(lib.tlor_fleet_capacity_kg(self._handle), (self.truck_count,), np.float64)

    @property
    def max_work_s(self):
        return read(lib.tlor_fleet_max_work_s(self._handle), (self.truck_count,), np.float64)

    @property
    def available(self):
        return read(lib.tlor_fleet_available(self._handle), (self.day_count, self.truck_count), bool)


class State(Handle):
    _destroy = "tlor_state_destroy"

    def __init__(self, node_count, level_kg=None, last_collection=None):
        levels = None if level_kg is None else as_doubles(level_kg, (node_count,), "level_kg")
        last = None
        if last_collection is not None:
            last = np.ascontiguousarray(last_collection, dtype=np.int32)
            if last.shape != (node_count,):
                raise InvalidArgument(1, f"last_collection has shape {last.shape}, expected {(node_count,)}")
        super().__init__(create(
            lib.tlor_state_create, int(node_count),
            None if levels is None else pointer(levels, c_double),
            None if last is None else pointer(last, c_int32),
        ))

    @classmethod
    def initial(cls, instance):
        return cls(instance.node_count)

    @classmethod
    def copy_of(cls, handle):
        n = lib.tlor_state_node_count(handle)
        return cls(n, read(lib.tlor_state_level_kg(handle), (n,), np.float64),
                   read(lib.tlor_state_last_collection(handle), (n,), np.int32))

    @property
    def node_count(self):
        return lib.tlor_state_node_count(self._handle)

    @property
    def level_kg(self):
        return read(lib.tlor_state_level_kg(self._handle), (self.node_count,), np.float64)

    @property
    def last_collection(self):
        return read(lib.tlor_state_last_collection(self._handle), (self.node_count,), np.int32)


class Plan(Handle):
    _destroy = "tlor_plan_destroy"

    def __init__(self, start, horizon):
        super().__init__(create(lib.tlor_plan_create, int(start), int(horizon)))

    @classmethod
    def copy_of(cls, handle):
        plan = cls(lib.tlor_plan_start(handle), lib.tlor_plan_horizon(handle))
        check_status(lib.tlor_plan_assign(plan._handle, handle))
        return plan

    @property
    def start(self):
        return lib.tlor_plan_start(self._handle)

    @property
    def horizon(self):
        return lib.tlor_plan_horizon(self._handle)

    def add(self, day, point):
        check_status(lib.tlor_plan_add(self._handle, int(day), int(point)))

    def remove(self, day, point):
        check_status(lib.tlor_plan_remove(self._handle, int(day), int(point)))

    def points(self, day):
        data = POINTER(c_int32)()
        count = c_size_t()
        check_status(lib.tlor_plan_points(self._handle, int(day), byref(data), byref(count)))
        return read(data, (count.value,), np.int32) if count.value else np.empty(0, dtype=np.int32)

    def days(self):
        result = {}
        for day in range(self.start, self.start + self.horizon):
            points = self.points(day)
            if len(points):
                result[day] = points.tolist()
        return result


class DayRoutes(Handle):
    _destroy = "tlor_day_routes_destroy"

    def __init__(self, day):
        super().__init__(create(lib.tlor_day_routes_create, int(day)))

    @classmethod
    def copy_of(cls, handle):
        routes = cls(lib.tlor_day_routes_day(handle))
        check_status(lib.tlor_day_routes_assign(routes._handle, handle))
        return routes

    @property
    def day(self):
        return lib.tlor_day_routes_day(self._handle)

    def add(self, truck, nodes):
        array = np.ascontiguousarray(nodes, dtype=np.int32)
        check_status(lib.tlor_day_routes_add(self._handle, int(truck), pointer(array, c_int32), array.size))

    def __len__(self):
        return lib.tlor_day_routes_count(self._handle)

    def __iter__(self):
        for index in range(len(self)):
            truck = c_int32()
            nodes = POINTER(c_int32)()
            count = c_size_t()
            check_status(lib.tlor_day_routes_get(self._handle, index, byref(truck), byref(nodes), byref(count)))
            yield truck.value, [nodes[i] for i in range(count.value)]


class Solution(Handle):
    _destroy = "tlor_solution_destroy"

    def __init__(self, plan):
        super().__init__(create(lib.tlor_solution_create, plan._handle))

    @classmethod
    def copy_of(cls, handle):
        solution = cls(Plan.copy_of(lib.tlor_solution_plan(handle)))
        check_status(lib.tlor_solution_assign(solution._handle, handle))
        return solution

    @property
    def plan(self):
        return Plan.copy_of(lib.tlor_solution_plan(self._handle))

    def set_routes(self, routes):
        check_status(lib.tlor_solution_set_routes(self._handle, routes._handle))

    def routes(self, day):
        handle = lib.tlor_solution_routes(self._handle, int(day))
        return None if not handle else DayRoutes.copy_of(handle)
