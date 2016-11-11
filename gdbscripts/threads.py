import gdb
import tailqueue

def pretty_thread(thr):
    return str(thr['td_name'].string())

def get_runnable_threads():
    runq = gdb.parse_and_eval('runq.rq_queues')
    rq_size = gdb.parse_and_eval('sizeof(runq.rq_queues)/sizeof(runq.rq_queues[0])')
    threads = []
    for i in range(0, rq_size):
        threads = threads + tailqueue.collect_values(runq[i], 'td_runq')
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
        print 'context switch from ', pretty_thread(frm), ' to ', pretty_thread(to)
        return self.stop_on

    def set_stop_on(self, arg):
        self.stop_on = arg

class CtxSwitchTracer():
    def __init__(self):
        self.ctxswitchbp = None

    def toggle(self):
        if self.ctxswitchbp:
            print 'cs tracing off'
            self.ctxswitchbp.delete()
            self.ctxswitchbp = None
        else:
            print 'cs tracing on'
            self.ctxswitchbp = CtxSwitchTracerBP()

class CreateThreadTracerBP(gdb.Breakpoint):
    def __init__(self):
        super(CreateThreadTracerBP, self).__init__('thread_create')
        self.stop_on = True

    def stop(self):
        td_name = gdb.newest_frame().read_var('name').string()
        print 'New thread in system: ', td_name
        return self.stop_on

    def stop_on_switch(self, arg):
        self.stop_on = arg

class CreateThreadTracer():
    def __init__(self):
        self.createThreadbp = None

    def toggle(self):
        if self.createThreadbp:
            print 'new thread trace off'
            self.createThreadbp.delete()
            self.createThreadbp = None
        else:
            print 'new thread trace on'
            self.createThreadbp = CreateThreadTracerBP()

class KernelThreads():
    def invoke(self):
        runnable = get_runnable_threads()
        if runnable:
            print "runnable_threads: ", map(pretty_thread, runnable)
        else:
            print "no runnable threads"
        print "current_thread: ", pretty_thread(current_thread())

