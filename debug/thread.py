import gdb

from .tailq import TailQueue
from .utils import (GdbStructMeta, OneArgAutoCompleteMixin, ProgramCounter,
                    enum, func_ret_addr, local_var, print_exception, TextTable)
from .ctx import Context


class Thread(metaclass=GdbStructMeta):
    __ctype__ = 'struct thread'
    __cast__ = {'td_waitpt': ProgramCounter,
                'td_tid': int,
                'td_state': enum,
                'td_prio': int,
                'td_name': lambda x: x.string()}

    @staticmethod
    def pointer_type():
        return gdb.lookup_type('struct thread')

    @classmethod
    def current(cls):
        return cls(gdb.parse_and_eval('_pcpu_data->curthread').dereference())

    @classmethod
    def from_pointer(cls, ptr):
        return cls(gdb.parse_and_eval('(struct thread *)' + ptr).dereference())

    @staticmethod
    def dump_list(threads):
        curr_tid = Thread.current().td_tid
        table = TextTable(types='ittit', align='rrrrl')
        table.header(['Id', 'Name', 'State', 'Priority', 'Waiting Point'])
        for td in threads:
            marker = '(*) ' if curr_tid == td.td_tid else ''
            table.add_row(['{}{}'.format(marker, td.td_tid), td.td_name,
                           td.td_state, td.td_prio, td.td_waitpt])
        print(table)

    @classmethod
    def list_all(cls):
        threads = TailQueue(gdb.parse_and_eval('all_threads'), 'td_all')
        return map(Thread, threads)

    def __repr__(self):
        return 'thread{%s/%d}' % (self.td_name, self.td_tid)


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


class Kthread(gdb.Command, OneArgAutoCompleteMixin):
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
        super().__init__('kthread', gdb.COMMAND_USER)

    def find_by_name(self, name):
        found = filter(lambda td: td.td_name == name, Thread.list_all())

        if len(found) > 1:
            print('Warning! There is more than 1 thread with name ', name)
        elif len(found) > 0:
            return found[0]
        else:
            print('Can\'t find thread with name="%s"!' % name)

    def find_by_id(self, tid):
        for td in Thread.list_all():
            if td.td_tid == tid:
                return td
        print('Can\'t find thread with tid=%d!' % tid)

    def dump_all(self):
        print('(*) current thread marker')
        Thread.dump_list(Thread.list_all())

    def dump_one(self, found):
        print(found.dump())
        print('\n>>> backtrace for %s' % found)
        ctx = Context()
        ctx.save()
        Context.load(found.td_kctx)
        gdb.execute('backtrace')
        ctx.restore()

    @print_exception
    def invoke(self, args, from_tty):
        if len(args) < 1:
            # give simplified view of all threads in the system
            self.dump_all()
        else:
            try:
                found = self.find_by_id(int(args))
            except ValueError:
                found = self.find_by_name(args)
            self.dump_one(found)

    def options(self):
        threads = Thread.list_all()
        return sum([map(lambda td: td.td_name, threads),
                    map(lambda td: td.td_tid, threads)], [])


class CurrentThread(gdb.Function):
    """Return address of currently running thread."""

    def __init__(self):
        super().__init__('thread')

    def invoke(self):
        return gdb.parse_and_eval('_pcpu_data->curthread')
