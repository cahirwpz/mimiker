import gdb

from .cmd import SimpleCommand, AutoCompleteMixin
from .struct import enum, cstr, GdbStructMeta, ProgramCounter, TailQueue
from .utils import func_ret_addr, local_var, TextTable
from .ctx import Context


class Thread(metaclass=GdbStructMeta):
    __ctype__ = 'struct thread'
    __cast__ = {'td_waitpt': ProgramCounter,
                'td_tid': int,
                'td_state': enum,
                'td_prio': int,
                'td_name': cstr}

    @staticmethod
    def current():
        return gdb.parse_and_eval('_pcpu_data->curthread')

    @classmethod
    def from_current(cls):
        return cls(Thread.current().dereference())

    @classmethod
    def from_pointer(cls, ptr):
        return cls(gdb.parse_and_eval('(struct thread *)' + ptr).dereference())

    @classmethod
    def list_all(cls):
        return map(cls, TailQueue(gdb.parse_and_eval('all_threads'), 'td_all'))

    @classmethod
    def find_by_name(cls, name):
        return [td for td in cls.list_all() if td.td_name == name]

    @classmethod
    def find_by_tid(cls, tid):
        for td in cls.list_all():
            if td.td_tid == tid:
                return td

    def __repr__(self):
        return 'thread{%s/%s}' % (self.td_name, self.td_tid)


class ThreadSwitchBP(gdb.Breakpoint):
    def __init__(self):
        super().__init__('ctx_switch')

    def stop(self):
        td_from = Thread.from_pointer('$a0')
        td_to = Thread.from_pointer('$a1')
        print('context switch from {} to {}'.format(td_from, td_to))


class ThreadCreateBP(gdb.Breakpoint):
    def __init__(self):
        super().__init__('*0x%x' % func_ret_addr('thread_create'))

    def stop(self):
        print('New', local_var('td').dereference(), 'in the system!')


class Kthread(SimpleCommand, AutoCompleteMixin):
    """dump info about threads

    Thread can be either specified by its identifier (td_tid) or by its name
    (td_name).

    Example:

    Given list of threads in system:
    id   name              state
    0    main              TDS_RUNNING
    1    kernel-thread-1   TDS_READY
    2    user-thread-1     TDS_READY
    3    user-thread-2     TDS_RUNNING

    User might print information about thread 1 by typing:
    $ kthread 1
    or
    $ kthread kernel-thread-1
    If there are more threads with the same name, gdb will print a warning.
    """

    def __init__(self):
        super().__init__('kthread')

    def find_by_name(self, name):
        found = Thread.find_by_name(name)
        if len(found) == 1:
            return found[0]
        if len(found) > 1:
            print('More than one thread with name %s"' % name)
        if len(found) == 0:
            print("Can't find thread with name='%s'!" % name)

    def find_by_tid(self, tid):
        found = Thread.find_by_tid(tid)
        if not found:
            print("Can't find thread with tid=%d!" % tid)
        return found

    def dump_all(self, backtrace=False):
        cur_td = Thread.from_current()
        table = TextTable(types='ittit', align='rrrrl')
        table.header(['Id', 'Name', 'State', 'Priority', 'Waiting Point'])
        for td in Thread.list_all():
            marker = '(*) ' if cur_td.td_tid == td.td_tid else ''
            table.add_row(['{}{}'.format(marker, td.td_tid), td.td_name,
                           td.td_state, td.td_prio, td.td_waitpt])
        print('(*) current thread marker')
        print(table)

        if backtrace:
            for td in Thread.list_all():
                state = str(td.td_state)
                if state not in ['TDS_INACTIVE', 'TDS_RUNNING', 'TDS_DEAD']:
                    self.dump_backtrace(td)

    def dump_backtrace(self, thread):
        # Don't switch context if this is the current thread.
        not_current = Thread.from_current().td_tid != thread.td_tid
        if not_current:
            ctx = Context.current()
            Context.load(Context.from_kctx(thread.td_kctx))
        print('\n>>> backtrace for %s' % thread)
        gdb.execute('backtrace')
        if not_current:
            Context.load(ctx)

    def dump_one(self, found):
        try:
            thread = self.find_by_tid(int(found))
        except ValueError:
            thread = self.find_by_name(found)
        if not thread:
            return
        print(thread.dump())
        self.dump_backtrace(thread)

    def __call__(self, arg):
        if arg and arg != '*':
            self.dump_one(arg)
        else:
            # give simplified view of all threads in the system
            self.dump_all(arg == '*')

    def options(self):
        threads = Thread.list_all()
        return sum([map(lambda td: td.td_name, threads),
                    map(lambda td: td.td_tid, threads)], [])


class CurrentThread(gdb.Function):
    """Return address of currently running thread."""

    def __init__(self):
        super().__init__('thread')

    def invoke(self):
        return Thread.current()
