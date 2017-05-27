from __future__ import print_function

import gdb
import tailq
import ptable


def thread_name(thr):
    return str(thr['td_name'].string())


def thread_id(thr):
    return str(thr['td_tid'])


def thread_state(thr):
    return str(thr['td_state'])


def get_all_threads():
    tdq = gdb.parse_and_eval('all_threads')
    return tailq.collect_values(tdq, 'td_all')


def current_thread():
    return gdb.parse_and_eval('thread_self()')


class CtxSwitchTracerBP(gdb.Breakpoint):

    def __init__(self):
        super(CtxSwitchTracerBP, self).__init__('ctx_switch')
        self.stop_on = False

    def stop(self):
        frm = gdb.parse_and_eval('(struct thread*)$a0').dereference()
        to = gdb.parse_and_eval('(struct thread*)$a1').dereference()
        print('context switch from {} to {}'.format(thread_name(frm),
                                                    thread_name(to)))
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


def dump_threads(threads):
    extractors = [thread_id, thread_name, thread_state]
    rows = [['Id', 'Name', 'State']]
    column_sizes = [0, 0, 0]
    for thread in threads:
        row = [f(thread) for f in extractors]
        rows.append(row)
    ptable.ptable(rows, fmt="llll", header=True)


class KernelThreads():

    def invoke(self):
        dump_threads(get_all_threads())
        print('current thread id: %s' % thread_id(current_thread()))


class ThreadPrettyPrinter():
    """
    Pretty prints single thread.
    Note that this prints ALL fields of thread. This looks OK for now because
    thread structure doesn't have that many fields, but in future we might want
    to distinguish between scheduler, process, etc. sets of fields.
    """

    def __init__(self, val):
        self.val = val

    def to_string(self):
        res = ['%s = %s' % (field, self.val[field]) for field in self.val.type]
        return "\n".join(res)

    def display_hint(self):
        return 'map'


def addThreadPrettyPrinter():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("threads")
    pp.add_printer('thread', 'thread', ThreadPrettyPrinter)
    gdb.printing.register_pretty_printer(gdb.current_objfile(), pp)


class Kthread(gdb.Command):
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

        # First try to search by thread_id
        try:
            int(args)
        except ValueError:
            # If can't find by thread id, search by name
            tds = filter(lambda t: thread_name(t) == args, get_all_threads())
            if len(tds) > 1:
                print("Warning! There is more than 1 thread with name ", args)
            if len(tds) > 0:
                print(tds[0])
                return

        for t in get_all_threads():
            if thread_id(t) == args:
                print(t)
                return

        print("Can't find thread with '", args, "' id or name")

    def complete(self, text, word):
        args = text.split()
        all_names = sum([map(thread_name, get_all_threads()),
                         map(thread_id, get_all_threads())], [])
        if len(args) == 0:
            return all_names
        if len(args) >= 2:
            return []
        suggestions = []
        for k in all_names:
            if k.startswith(args[0], 0, len(k) - 1):
                suggestions.append(k)
        return suggestions


addThreadPrettyPrinter()
Kthread()
