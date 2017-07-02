import gdb
import threads


class Ktrace(gdb.Command):
    def __init__(self):
        super(Ktrace, self).__init__('ktrace', gdb.COMMAND_USER)
        self.tracepoint = {
            'thread-create': threads.CreateThreadTracer(),
            'ctx-switch': threads.CtxSwitchTracer()
        }

    def invoke(self, args, from_tty):
        if len(args) < 1:
            raise(gdb.GdbError('Usage: ktrace [tracepoint]. Tracepoints: {}.'
                               .format(self.tracepoint.keys())))
        if args not in self.tracepoint:
            raise gdb.GdbError("No such tracepoint - {}.".format(args))
        self.tracepoint[args].toggle()

    def complete(self, text, word):
        args = text.split()
        if len(args) == 0:
            return self.tracepoint.keys()
        if len(args) >= 2:
            return []
        suggestions = []
        for k in self.tracepoint.keys():
            if k.startswith(args[0], 0, len(k) - 1):
                suggestions.append(k)
        return suggestions

Ktrace()
