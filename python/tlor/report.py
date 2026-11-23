from contextlib import contextmanager

from ._lib import lib, text
from .errors import check_status
from .model import Handle, create, fs_path


class Report(Handle):
    _destroy = "tlor_report_destroy"

    def __init__(self, path=None, **context):
        super().__init__(create(lib.tlor_report_create))
        self.path = path
        for key, value in context.items():
            check_status(lib.tlor_report_set_context(self._handle, text(key), text(value)))

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        if getattr(self, "path", None) is not None:
            self.write(self.path)
        return False

    @contextmanager
    def phase(self, name):
        check_status(lib.tlor_report_phase_begin(self._handle, text(name)))
        try:
            yield self
        finally:
            check_status(lib.tlor_report_phase_end(self._handle))

    def count(self, name, delta=1):
        check_status(lib.tlor_report_count(self._handle, text(name), int(delta)))

    def write(self, path):
        check_status(lib.tlor_report_write_json(self._handle, fs_path(path)))
