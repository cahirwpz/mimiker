import gdb

from .tailq import TailQueue
from .thread import Thread
from .utils import GdbStructMeta, ProgramCounter


class SleepQueue(metaclass=GdbStructMeta):
    __ctype__ = 'struct sleepq'

    def __init__(self, addr):
        self._obj = gdb.parse_and_eval("sleepq_lookup((void *)%d)" % addr)

    def __str__(self):
        if self._obj:
            blocked = map(Thread, TailQueue(self.sq_blocked, 'td_sleepq'))
            return '[%s]' % ', '.join(map(str, blocked))
        return '[]'


class Mutex(metaclass=GdbStructMeta):
    __ctype__ = 'struct mtx'
    __cast__ = {'m_count': int,
                'm_lockpt': ProgramCounter}

    def __str__(self):
        if self.m_owner:
            return 'mtx{owner = %s, lockpt = %s, count = %d, blocked = %s}' % (
                    Thread(self.m_owner.dereference()), self.m_lockpt,
                    self.m_count, SleepQueue(self._obj.address))
        return 'mtx{owner = None}'


class CondVar(metaclass=GdbStructMeta):
    __ctype__ = 'struct condvar'
    __cast__ = {'waiters': int}

    def __str__(self):
        return 'condvar{waiters[%d] = %s}' % (
                self.waiters, SleepQueue(self._obj.address))
