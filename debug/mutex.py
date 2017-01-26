import gdb
import sys
import os
sys.path.append(os.path.join(os.getcwd(), 'debug'))
import tailq


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
    return map(lambda n: n.string(),
               tailq.collect_values(sq['sq_blocked'], 'td_name'))


class MutexPrettyPrinter():

    def __init__(self, val):
        self.val = val

    def to_string(self):
        if self.val['m_owner'] == 0:
            return "Mutex unowned"
        return "{owner = %s, blocked = %s}" % (
            get_mutex_owner(self.val), get_threads_blocked_on_mutex(self.val))

    def display_hint(self):
        return 'map'


def addMutexPrettyPrinter():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("mutexes")
    pp.add_printer('mtx', 'mtx', MutexPrettyPrinter)
    gdb.printing.register_pretty_printer(gdb.current_objfile(), pp)

addMutexPrettyPrinter()
