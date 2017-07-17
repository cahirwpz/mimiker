import gdb
import thread
import utils


class Ktrace(gdb.Command, utils.OneArgAutoCompleteMixin):
    """TODO: documentation"""

    def __init__(self):
        super(Ktrace, self).__init__('ktrace', gdb.COMMAND_USER)

        self.tracepoint = {
            'thread-create': thread.CreateThreadTracer(),
            'ctx-switch': thread.CtxSwitchTracer()
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
