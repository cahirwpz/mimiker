import gdb

from .thread import CreateThreadTracer, CtxSwitchTracer
from .utils import OneArgAutoCompleteMixin


class Ktrace(gdb.Command, OneArgAutoCompleteMixin):
    """TODO: documentation"""

    def __init__(self):
        super(Ktrace, self).__init__('ktrace', gdb.COMMAND_USER)

        self.tracepoint = {
            'thread-create': CreateThreadTracer(),
            'ctx-switch': CtxSwitchTracer()
        }

    def invoke(self, args, from_tty):
        if len(args) < 1:
            raise gdb.GdbError('Usage: ktrace [tracepoint]. Tracepoints: {}.'
                               .format(self.tracepoint.keys()))
        if args not in self.tracepoint:
            raise gdb.GdbError("No such tracepoint - {}.".format(args))
        self.tracepoint[args].toggle()

    def options(self):
        return self.tracepoint.keys()
