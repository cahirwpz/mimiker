import gdb
import tailq


def thread_name(thr):
    return str(thr['td_name'].string())


def thread_id(thr):
    return str(thr['td_tid'])


def thread_state(thr):
    return str(thr['td_state'])


def get_all_threads():
    tdq = gdb.parse_and_eval('all_threads')
    threads = tailq.collect_values(tdq, 'td_all')
    return threads


def current_thread():
    return gdb.parse_and_eval('thread_self()')


class CtxSwitchTracerBP(gdb.Breakpoint):
    def __init__(self):
        super(CtxSwitchTracerBP, self).__init__('ctx_switch')
        self.stop_on = False

    def stop(self):
        frm = gdb.parse_and_eval('(struct thread*)$a0').dereference()
        to = gdb.parse_and_eval('(struct thread*)$a1').dereference()
        print('context switch from ', thread_name(frm), ' to ', thread_name(to))
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
    rows = [['id', 'name', 'state']]
    column_sizes = [0, 0, 0]
    for thread in threads:
        row = [f(thread) for f in extractors]
        rows.append(row)
    for r in rows:
        column_sizes = [max(a, b) for (a, b) in zip(map(len, r), column_sizes)]
    for r in rows:
        pretty_row = "   ".join([s.ljust(l) for (s, l) in zip(r, column_sizes)])
        print(pretty_row)

class KernelThreads():
    def invoke(self):
        threads = get_all_threads()
        dump_threads(threads)
        print ''
        print 'current thread id: ', thread_id(current_thread())
