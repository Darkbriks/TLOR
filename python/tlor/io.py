import datetime

from ._lib import lib, text
from .errors import InvalidArgument, check_status
from .model import fs_path, handle_of
from .operations import _demand, _demand_pointer


def _date_text(value):
    if isinstance(value, datetime.datetime):
        value = value.date()
    if isinstance(value, datetime.date):
        return value.isoformat()
    return str(value)


def write_solution(out_dir, instance, solution, state=None, demand_kg=None, first_date=None):
    date = instance.first_date if first_date is None else first_date
    if date is None:
        raise InvalidArgument(1, "first_date is required for an instance that was not loaded from files")
    demand = _demand(instance, demand_kg)
    check_status(lib.tlor_write_solution(fs_path(out_dir), instance._handle, handle_of(state), solution._handle,
                                         _demand_pointer(demand), text(_date_text(date))))


def write_routes_json(path, routes):
    check_status(lib.tlor_write_routes_json(fs_path(path), routes._handle))
