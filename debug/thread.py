from __future__ import print_function

import gdb
from tailq import TailQueue
import utils
import ptable
import re


class ProgramCounter():
    def __init__(self, pc):
        self.pc = pc.cast(gdb.lookup_type('unsigned long'))

    def __str__(self):
        if self.pc == 0:
            return '-'
        line = gdb.execute('info line *0x%x' % self.pc, to_string=True)
        m = re.match(r'Line (\d+) of "(.*)"', line)
        m = m.groups()
        return "%s:%s" % (m[1], m[0])


class Thread(utils.PrettyPrinterMixin):
    def __init__(self, td):
        self._obj = td
        self.name = td['td_name'].string()
        self.tid = int(td['td_tid'])
        self.state = str(td['td_state'])
        self.waitpt = ProgramCounter(td['td_waitpt'])

    @staticmethod
    def pointer_type():
        return gdb.lookup_type('struct thread')

    @classmethod
    def current(cls):
        return cls(gdb.parse_and_eval('thread_self()'))

    @classmethod
    def from_pointer(cls, ptr):
        return cls(gdb.parse_and_eval('(struct thread *)' + ptr).dereference())

    @staticmethod
    def dump_list(threads):
        rows = [['Id', 'Name', 'State', 'Waiting Point']]
        curr_tid = Thread.current().tid
        rows.extend([['', '(*) '][curr_tid == td.tid] + str(td.tid), td.name,
                     td.state, str(td.waitpt)] for td in map(Thread, threads))
        ptable.ptable(rows, fmt="rlll", header=True)

    @classmethod
    def list_all(cls):
        threads = TailQueue(gdb.parse_and_eval('all_threads'), 'td_all')
        return map(Thread, threads)

    def __str__(self):
        return 'thread{tid=%d, name="%s"}' % (self.tid, self.name)

    def dump(self):
        res = ['%s = %s' % (field, self._obj[field])
               for field in self._obj.type]
        return '\n'.join(res)

    def display_hint(self):
        return 'map'


class CtxSwitchTracerBP(gdb.Breakpoint):

    def __init__(self):
        super(CtxSwitchTracerBP, self).__init__('ctx_switch')
        self.stop_on = False

    def stop(self):
        td_from = Thread.from_pointer('$a0')
        td_to = Thread.from_pointer('$a1')
        print('context switch from {} to {}'.format(td_from, td_to))
        return self.stop_on

    def set_stop_on(self, arg):
        self.stop_on = arg


class CtxSwitchTracer():

    def __init__(self):
        self.ctxswitchbp = None

    def toggle(self):
        if self.ctxswitchbp:
            print('context switch tracing off')
            self.ctxswitchbp.delete()
            self.ctxswitchbp = None
        else:
            print('context switch tracing on')
            self.ctxswitchbp = CtxSwitchTracerBP()


class CreateThreadTracerBP(gdb.Breakpoint):

    def __init__(self):
        super(CreateThreadTracerBP, self).__init__('thread_create')
        self.stop_on = True

    def stop(self):
        td_name = gdb.newest_frame().read_var('name').string()
        print('New thread in system: ', td_name)
        return self.stop_on

    def stop_on_switch(self, arg):
        self.stop_on = arg


class CreateThreadTracer():

    def __init__(self):
        self.createThreadbp = None

    def toggle(self):
        if self.createThreadbp:
            print('create-thread trace off')
            self.createThreadbp.delete()
            self.createThreadbp = None
        else:
            print('create-thread trace on')
            self.createThreadbp = CreateThreadTracerBP()


class KernelThreads():

    def invoke(self):
        print('(*) current thread marker')
        Thread.dump_list(Thread.list_all())


class Kthread(gdb.Command, utils.OneArgAutoCompleteMixin):
    """
    kthread prints info about given thread. Thread can be either given by
    its identifier (td_tid) or by its name (td_name). This command uses
    thread pretty printer to print thread structure, and supports tab
    auto completion.
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
        super(Kthread, self).__init__("kthread", gdb.COMMAND_USER)

    def invoke(self, args, from_tty):
        if len(args) < 1:
            raise(gdb.GdbError('Usage: kthread [td_name|td_tid]'))

        threads = Thread.list_all()

        # First try to search by thread_id
        try:
            int(args)
        except ValueError:
            # If can't find by thread id, search by name
            tds = filter(lambda td: td.name == args, threads)
            if len(tds) > 1:
                print("Warning! There is more than 1 thread with name ", args)
            if len(tds) > 0:
                print(tds[0].dump())
                return

        for td in threads:
            if td.tid == args:
                print(td.dump())
                return

        print("Can't find thread with '%s' id or name" % args)

    def options(self):
        threads = Thread.list_all()
        return sum([map(lambda td: td.name, threads),
                    map(lambda td: td.tid, threads)], [])
