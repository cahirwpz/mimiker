import gdb

from .cmd import SimpleCommand
from .utils import TextTable, global_var
from .struct import GdbStructMeta, TailQueue, enum
from .thread import Thread
from .vm_map import VmMap
from .sync import Mutex


class Process(metaclass=GdbStructMeta):
    __ctype__ = 'struct proc'
    __cast__ = {'p_pid': int,
                'p_lock': Mutex,
                'p_thread': Thread,
                'p_uspace': VmMap,
                'p_state': enum}

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

    @classmethod
    def find_by_pid(cls, pid):
        for p in cls.list_all():
            if p.p_pid == pid:
                return p

    def __repr__(self):
        return 'proc{pid=%d}' % self.p_pid


class Kprocess(SimpleCommand):
    """List all processes."""

    def __init__(self):
        super().__init__('kproc')

    def __call__(self, args):
        table = TextTable(align='rllllr')
        table.header(['Pid', 'Tid', 'State', 'SigPend', 'SigMask',
                      'Main lock state'])
        for p in Process.list_all():
            if p.p_state == 'PS_ZOMBIE':
                td, sigpend, sigmask = None, 0, 0
            else:
                td = p.p_thread
                sigpend, sigmask = td.td_sigpend, td.td_sigmask
            table.add_row([p.p_pid, td, p.p_state, sigpend, sigmask, p.p_lock])
        print(table)


class CurrentProcess(gdb.Function):
    """Return address of currently running process."""

    def __init__(self):
        super().__init__('process')

    def invoke(self):
        return Process.current()
