import gdb
from tailq import TailQueue
from thread import Thread
import utils


class CondVar(object):
    __metaclass__ = utils.GdbValueMeta
    __ctype__ = 'struct condvar'
    __cast__ = {'waiters': int}

    def __init__(self, cv):
        self._obj = cv

    def list_waiters(self):
        sq = gdb.parse_and_eval('sleepq_lookup((void *)%d)' % self._obj.address)
        if sq == 0:
            return None
        return ', '.join(map(str, TailQueue(sq['sq_blocked'], 'td_sleepq')))

    def __str__(self):
        return 'condvar{waiters[%d] = [%s]}' % (
                self.waiters, self.list_waiters())
