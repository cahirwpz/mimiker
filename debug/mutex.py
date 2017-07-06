import gdb
from tailq import TailQueue


def get_mutex_owner(mtx):
    state = mtx['m_owner']
    td_pointer = gdb.lookup_type('struct thread').pointer()
    owner = state.cast(td_pointer)
    if owner:
        owner = owner.dereference()
        return owner['td_name'].string()
    return None


def get_threads_blocked_on_mutex(mtx):
    sq = gdb.parse_and_eval("sleepq_lookup((void *)%d)" % mtx.address)
    return map(lambda n: n.string(), TailQueue(sq['sq_blocked'], 'td_name'))


class PrettyPrinter():
    def __init__(self, val):
        self.val = val

    def to_string(self):
        if self.val['m_owner'] == 0:
            return "Mutex unowned"
        return "{owner = %s, blocked = %s}" % (
            get_mutex_owner(self.val), get_threads_blocked_on_mutex(self.val))

    def display_hint(self):
        return 'map'
