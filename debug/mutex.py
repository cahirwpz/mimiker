import gdb
from tailq import TailQueue
from thread import Thread
import utils


class Mutex(utils.PrettyPrinterMixin):
    def __init__(self, mtx):
        self._obj = mtx
        self.typ = mtx['m_type']
        self.count = mtx['m_count']
        self.owner = None

        owner = mtx['m_owner']
        if owner:
            self.owner = owner.dereference()

    def list_blocked(self):
        sq = gdb.parse_and_eval("sleepq_lookup((void *)%d)" %
                                self._obj.address)
        if sq == 0:
            return None
        return map(Thread, TailQueue(sq['sq_blocked'], 'td_sleepq'))

    def __str__(self):
        if self.owner is None:
            return 'mtx{owner = None}'
        return 'mtx{owner=%s, count=%d, blocked=%s}' % (
                Thread(self.owner), self.count, self.list_blocked())

    def display_hint(self):
        return 'map'
