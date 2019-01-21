import gdb

from .thread import ThreadCreateBP, ThreadSwitchBP
from .utils import OneArgAutoCompleteMixin


class Tracer():
    def __init__(self, breakpoint, description):
        self.__instance = None
        self.description = description
        self.breakpoint = breakpoint

    def toggle(self):
        if self.__instance:
            print('{} tracing off'.format(self.description))
            self.__instance.delete()
            self.__instance = None
        else:
            print('{} tracing on'.format(self.description))
            self.__instance = self.breakpoint()


class Ktrace(gdb.Command, OneArgAutoCompleteMixin):
    """TODO: documentation"""

    def __init__(self):
        super().__init__('ktrace', gdb.COMMAND_USER)

        self.tracepoint = {
            'thread-create': Tracer(ThreadCreateBP, "thread create"),
            'thread-switch': Tracer(ThreadSwitchBP, "thread switch")
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
