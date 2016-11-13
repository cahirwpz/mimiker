import gdb
import sys
import os
sys.path.append(os.path.join(os.getcwd(), 'debug'))
import tailq


def get_mutex_owner(mtx):
    state = mtx['mtx_state']
    td_pointer = gdb.lookup_type('struct thread').pointer()
    owner = state.cast(td_pointer)
    if owner:
        owner = owner.dereference()
        return owner['td_name'].string()
    return None


def get_threads_blocked_on_mutex(mtx):
    ts = mtx['turnstile']
    return map(lambda t: t['td_name'].string(), tailq.collect_values(ts['td_queue'], 'td_lock'))


def pretty_list_of_string(lst):
    return "[" + ", ".join(lst) + "]"


class MutexPrettyPrinter():

    def __init__(self, val):
        self.val = val

    def to_string(self):
        res = []
        if self.val['mtx_state'] == 0:
            return "Mutex unowned"
        else:
            res.append("Owner = " + str(get_mutex_owner(self.val)))
            res.append("Waiting for access = " +
                       pretty_list_of_string(get_threads_blocked_on_mutex(self.val)))
        return "\n".join(res)

    def display_hint(self):
        return 'map'


def addMutexPrettyPrinter():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("mutexes")
    pp.add_printer('mtx', 'mtx', MutexPrettyPrinter)
    gdb.printing.register_pretty_printer(gdb.current_objfile(), pp)

addMutexPrettyPrinter()
