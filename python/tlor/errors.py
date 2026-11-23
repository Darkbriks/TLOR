from ._lib import lib


class Error(Exception):
    def __init__(self, status, message):
        super().__init__(message)
        self.status = status
        self.message = message


class InvalidArgument(Error):
    pass


class UnknownMethod(Error):
    pass


class InvalidParam(Error):
    pass


class IOFailure(Error):
    pass


class Infeasible(Error):
    pass


class CallbackError(Error):
    pass


class InternalError(Error):
    pass


_BY_STATUS = {
    1: InvalidArgument,
    2: UnknownMethod,
    3: InvalidParam,
    4: IOFailure,
    5: Infeasible,
    6: CallbackError,
    7: InternalError,
}


def check_status(status):
    if status != 0:
        message = (lib.tlor_last_error() or b"").decode("utf-8", "replace")
        raise _BY_STATUS.get(status, Error)(status, message)
