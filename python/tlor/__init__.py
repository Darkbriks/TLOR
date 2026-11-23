from ._lib import lib as _lib
from .errors import (CallbackError, Error, Infeasible, InternalError, InvalidArgument, InvalidParam, IOFailure,
                     UnknownMethod)
from .model import DayRoutes, Fleet, Instance, Params, Plan, Solution, State
from .operations import (Evaluation, RollingResult, Violation, check, evaluate, improve, indicator_names, route,
                         run_rolling, schedule, simulate)
from .registry import (ImproveRequest, MethodInfo, ParamSpec, RouteRequest, ScheduleRequest, methods,
                       register_method, unregister_method)
from .report import Report

__version__ = _lib.tlor_version().decode()
