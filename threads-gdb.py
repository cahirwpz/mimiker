import gdb

def pretty_thread(thr):
    return str(thr['td_name'].string())

def collect_values_from_tq(tq,field):
    tq_it = tq['tqh_first']
    ret = []
    while tq_it != 0:
        tq_it = tq_it.dereference() #tq_it is now value
        ret.append(tq_it)
        tq_it = tq_it[field]['tqe_next']
    return ret

def get_runnable_threads():
    runq = gdb.parse_and_eval('runq.rq_queues')
    rq_size = gdb.parse_and_eval('sizeof(runq.rq_queues)/sizeof(runq.rq_queues[0])')
    threads = []
    for i in range(0,rq_size):
        threads = threads + collect_values_from_tq(runq[i], 'td_runq')
    return threads

def current_thread():
    return gdb.parse_and_eval('thread_self()')

class KernelThreads (gdb.Command):
    def __init__(self):
        super(KernelThreads, self).__init__("kernel-threads", gdb.COMMAND_USER)

    def invoke(self, args, from_tty):
        argv = gdb.string_to_argv(args)
        if len(argv) != 0:
            raise gdb.GdbError("kernel-threads takes no arguments")
        runnable = get_runnable_threads()
        if runnable:
            print "runnable_threads: ", map(pretty_thread, runnable)
        else:
            print "no runnable threads"
        print "current_thread: ", pretty_thread(current_thread())

class CtxSwitchTracerBP (gdb.Breakpoint):
    def __init__(self):
        super(CtxSwitchTracerBP, self).__init__('ctx_switch')
        self.stop_on = False

    def stop(self):
        frm = gdb.parse_and_eval('(struct thread*)$a0').dereference()
        to = gdb.parse_and_eval('(struct thread*)$a1').dereference()
        print 'context switch from ', pretty_thread(frm), ' to ', pretty_thread(to)
        return self.stop_on

    def set_stop_on(self,arg):
        self.stop_on = arg

class CtxSwitchTracer (gdb.Command):
    def __init__(self):
        super(CtxSwitchTracer, self).__init__('cs-trace', gdb.COMMAND_USER)
        self.ctxswitchbp = None

    def invoke(self, args, from_tty):
        if(self.ctxswitchbp and len(args) > 0):
            if(args == 'stop'):
                self.ctxswitchbp.set_stop_on(True)
                return
            if(args == 'nostop'):
                self.ctxswitchbp.set_stop_on(False)
                return
            print 'cs-trace invalid arg: only stop and nostop allowed'
            return

        if(self.ctxswitchbp):
            print('cs tracing off')
            self.ctxswitchbp.delete()
            self.ctxswitchbp = None
        else:
            print('cs tracing on')
            self.ctxswitchbp = CtxSwitchTracerBP()

class NewThreadTracerBP (gdb.Breakpoint):
    def __init__(self):
        super(NewThreadTracerBP, self).__init__('thread_create')
        self.stop_on = True

    def stop(self):
        td_name = gdb.newest_frame().read_var('name').string()
        print 'New thread in system: ', td_name
        return self.stop_on

    def stop_on_switch(self,arg):
        self.stop_on = arg

class NewThreadTracer (gdb.Command):
    def __init__(self):
        super(NewThreadTracer, self).__init__('new-thread-trace', gdb.COMMAND_USER)
        self.newthreadbp = None

    def invoke(self, args, from_tty):
        if(self.newthreadbp):
            print('new thread trace off')
            self.newthreadbp.delete()
            self.newthreadbp = None
        else:
            print('new thread trace on')
            self.newthreadbp = NewThreadTracerBP()


KernelThreads()
CtxSwitchTracer()
NewThreadTracer()

