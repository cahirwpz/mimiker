import gdb

from .cmd import SimpleCommand, AutoCompleteMixin
from .utils import TextTable, global_var
from .struct import GdbStructMeta, TailQueue, enum
from .thread import Thread


class Process(metaclass=GdbStructMeta):
    __ctype__ = 'struct proc'
    __cast__ = {'p_pid': int, 'p_thread': Thread, 'p_state': enum}

    @staticmethod
    def current():
        return gdb.parse_and_eval('_pcpu_data->curthread->td_proc')

    @classmethod
    def from_current(cls):
        return cls(Process.current().dereference())

    @classmethod
    def from_pointer(cls, ptr):
        return cls(gdb.parse_and_eval('(struct proc *)' + ptr).dereference())

    @classmethod
    def list_all(cls):
        alive = TailQueue(global_var('proc_list'), 'p_all')
        dead = TailQueue(global_var('zombie_list'), 'p_all')
        return map(cls, list(alive) + list(dead))

    def __repr__(self):
        return 'proc{pid=%d}' % self.p_pid


class Kprocess(SimpleCommand):
    """List all processes."""

    def __init__(self):
        super().__init__('kproc')

    def __call__(self, args):
        table = TextTable(align='rll')
        table.header(['Pid', 'Tid', 'State'])
        for p in Process.list_all():
            thread = None if p.p_state == 'PS_ZOMBIE' else p.p_thread
            table.add_row([p.p_pid, thread, p.p_state])
        print(table)


class CurrentProcess(gdb.Function):
    """Return address of currently running process."""

    def __init__(self):
        super().__init__('process')

    def invoke(self):
        return Process.current()
