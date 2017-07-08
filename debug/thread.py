from __future__ import print_function

import gdb
from tailq import TailQueue
import utils
import ptable
import re
import traceback
from ctx import Context


class ProgramCounter():
    def __init__(self, pc):
        self.pc = pc.cast(gdb.lookup_type('unsigned long'))

    def __str__(self):
        if self.pc == 0:
            return 'null'
        line = gdb.execute('info line *0x%x' % self.pc, to_string=True)
        m = re.match(r'Line (\d+) of "(.*)"', line)
        m = m.groups()
        return '%s:%s' % (m[1], m[0])


class Thread(object):
    __metaclass__ = utils.GdbStructMeta
    __ctype__ = 'struct thread'
    __cast__ = {'td_waitpt': ProgramCounter,
                'td_tid': int,
                'td_state': str,
                'td_name': lambda x: x.string()}

    @staticmethod
    def pointer_type():
        return gdb.lookup_type('struct thread')

    @classmethod
    def current(cls):
        return cls(gdb.parse_and_eval('thread_self()').dereference())

    @classmethod
    def from_pointer(cls, ptr):
        return cls(gdb.parse_and_eval('(struct thread *)' + ptr).dereference())

    @staticmethod
    def dump_list(threads):
        rows = [['Id', 'Name', 'State', 'Waiting Point']]
        curr_tid = Thread.current().td_tid
        rows.extend([['', '(*) '][curr_tid == td.td_tid] + str(td.td_tid),
                     td.td_name, td.td_state, str(td.td_waitpt)]
                    for td in threads)
        ptable.ptable(rows, fmt='rlll', header=True)

    @classmethod
    def list_all(cls):
        threads = TailQueue(gdb.parse_and_eval('all_threads'), 'td_all')
        return map(Thread, threads)

    def __str__(self):
        return 'thread{tid=%d, name="%s"}' % (self.td_tid, self.td_name)


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
    """dump info about given thread including its backtrace

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
        super(Kthread, self).__init__("kthread", gdb.COMMAND_USER)

    def invoke(self, args, from_tty):
        if len(args) < 1:
            raise(gdb.GdbError('Usage: kthread [td_name|td_tid]'))

        threads = Thread.list_all()

        try:
            args = int(args)
        except ValueError:
            pass

        found = None

        if type(args) == unicode:
            tds = filter(lambda td: td.td_name == args, threads)
            if len(tds) > 1:
                print("Warning! There is more than 1 thread with name ", args)
            elif len(tds) > 0:
                found = tds[0]
            else:
                print('Can\'t find thread with name="%s"!' % args)
                return

        if type(args) == int:
            for td in threads:
                if td.td_tid == args:
                    found = td
                    break
            if not found:
                print('Can\'t find thread with tid=%d!' % args)
                return

        try:
            print(found.dump())
            print('\n>>> backtrace for %s' % found)
            ctx = Context()
            ctx.save()
            Context.load(td.td_kctx)
            gdb.execute('backtrace')
            ctx.restore()
        except:
            traceback.print_exc()

    def options(self):
        threads = Thread.list_all()
        return sum([map(lambda td: td.td_name, threads),
                    map(lambda td: td.td_tid, threads)], [])
