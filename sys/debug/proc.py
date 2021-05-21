import gdb

from .cmd import SimpleCommand, AutoCompleteMixin
from .utils import TextTable, global_var
from .struct import GdbStructMeta, TailQueue, enum
from .thread import Thread
from .sync import Mutex


class Process(metaclass=GdbStructMeta):
    __ctype__ = 'struct proc'
    __cast__ = {'p_pid': int,
                'p_lock': Mutex,
                'p_thread': Thread,
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
    def from_pid(cls, pid):
        alive = list(TailQueue(global_var('proc_list'), 'p_all'))
        dead = list(TailQueue(global_var('zombie_list'), 'p_all'))
        for p in alive + dead:
            if p['p_pid'] == pid:
                return cls(p)
        return None

    @classmethod
    def list_all(cls):
        alive = TailQueue(global_var('proc_list'), 'p_all')
        dead = TailQueue(global_var('zombie_list'), 'p_all')
        return map(cls, list(alive) + list(dead))

    def __repr__(self):
        return 'proc{pid=%d}' % self.p_pid

    def address(self):
        return self._obj.address


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


class ShowProcess(gdb.Function):
    """Return address of process of given pid (default current process)."""

    def __init__(self):
        super().__init__('proc')

    def invoke(self, pid=-1):
        if pid == -1:
            return Process.current()
        p = Process.from_pid(pid)
        if p:
            return p.address()
        print(f"No process of pid={pid}")
        return 0
