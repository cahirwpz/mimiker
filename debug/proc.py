import gdb

from .cmd import SimpleCommand, AutoCompleteMixin
from .utils import TextTable, global_var
from .struct import GdbStructMeta, TailQueue, enum
from .thread import Thread


class Process(metaclass=GdbStructMeta):
    __ctype__ = 'struct proc'
    __cast__ = {'p_pid': int, 'p_thread': Thread, 'p_state': enum}

    @staticmethod
    def pointer_type():
        return gdb.lookup_type('struct proc')

    @classmethod
    def current(cls):
        return cls(gdb.parse_and_eval('_pcpu_data->curthread->td_proc')
                   .dereference())

    @classmethod
    def from_pointer(cls, ptr):
        return cls(gdb.parse_and_eval('(struct proc *)' + ptr).dereference())

    @classmethod
    def list_all(cls):
        alive = TailQueue(global_var('proc_list'), 'p_all')
        dead = TailQueue(global_var('zombie_list'), 'p_all')
        return map(Process, list(alive) + list(dead))

    def __str__(self):
        return 'proc{pid=%d}' % self.p_pid


class Kprocess(SimpleCommand, AutoCompleteMixin):
    """List all processes."""

    def __init__(self):
        super().__init__('kproc')

    def dump_all(self):
        table = TextTable(align='rll')
        table.header(['Pid', 'Tid', 'State'])
        for p in Process.list_all():
            table.add_row([p.p_pid, p.p_thread, p.p_state])
        print(table)

    def __call__(self, args):
        self.dump_all()

    def options(self):
        return None
