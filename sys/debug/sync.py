import gdb

from .struct import TailQueue, GdbStructMeta, ProgramCounter
from .thread import Thread


class SleepQueue(metaclass=GdbStructMeta):
    __ctype__ = 'struct sleepq'

    def __init__(self, addr):
        self._obj = gdb.parse_and_eval("sleepq_lookup((void *)%d)" % addr)

    def __str__(self):
        if self._obj:
            blocked = map(Thread, TailQueue(self.sq_blocked, 'td_sleepq'))
            return '[%s]' % ', '.join(map(str, blocked))
        return '[]'


class Turnstile(metaclass=GdbStructMeta):
    __ctype__ = 'struct turnstile'

    def __init__(self, addr):
        self._obj = gdb.parse_and_eval("turnstile_lookup((void *)%d)" % addr)

    def __str__(self):
        if self._obj:
            blocked = map(Thread, TailQueue(self.ts_blocked, 'td_blockedq'))
            return '[%s]' % ', '.join(map(str, blocked))
        return '[]'


class Mutex(metaclass=GdbStructMeta):
    __ctype__ = 'struct mtx'
    __cast__ = {'m_count': int}

    def __str__(self):
        if self.m_owner:
            owner = self.m_owner & -8
            owner = owner.cast(gdb.lookup_type('thread_t').pointer())
            return 'mtx{owner = %s, count = %d, blocked = %s}' % (
                    Thread(owner.dereference()),
                    self.m_count, Turnstile(self._obj.address))
        return 'mtx{owner = None}'


class CondVar(metaclass=GdbStructMeta):
    __ctype__ = 'struct condvar'
    __cast__ = {'waiters': int}

    def __str__(self):
        return 'condvar{waiters[%d] = %s}' % (
                self.waiters, SleepQueue(self._obj.address))
