import gdb
from tailq import TailQueue
from thread import Thread
import utils


class Mutex(object):
    __metaclass__ = utils.GdbValueMeta
    __ctype__ = 'struct mtx'
    __cast__ = {'m_count': int}

    def list_blocked(self):
        sq = gdb.parse_and_eval("sleepq_lookup((void *)%d)" %
                                self._obj.address)
        if sq == 0:
            return None
        return map(Thread, TailQueue(sq['sq_blocked'], 'td_sleepq'))

    def __str__(self):
        if self.m_owner:
            return 'mtx{owner = %s, count = %d, blocked = %s}' % (
                    Thread(self.m_owner.dereference()), self.m_count,
                    self.list_blocked())
        return 'mtx{owner = None}'
