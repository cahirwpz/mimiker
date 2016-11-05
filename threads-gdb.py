import gdb

def print_thread(thr):
    print(thr['td_name'])

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
            print("runnable_threads:")
            for t in runnable:
                print_thread(t)
        else:
            print("no runnable threads")
        print("current_thread:")
        print_thread(current_thread())



KernelThreads()

