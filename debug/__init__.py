import gdb
import ktrace
import kdump
import klog
import cpu
import thread
import sync
import kmem

def addPrettyPrinters():
    pp = gdb.printing.RegexpCollectionPrettyPrinter('mimiker')
    pp.add_printer('mtx', 'mtx', sync.Mutex)
    pp.add_printer('condvar', 'condvar', sync.CondVar)
    pp.add_printer('thread', 'thread', thread.Thread)
    pp.add_printer('timeval', 'timeval', klog.TimeVal)
    gdb.printing.register_pretty_printer(gdb.current_objfile(), pp)

addPrettyPrinters()
ktrace.Ktrace()
thread.Kthread()
cpu.Cpu()
kdump.Kdump()
klog.Klog()
kmem.Kmem()
