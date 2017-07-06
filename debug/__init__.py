import gdb
import ktrace
import kdump
import klog
import mutex
import thread


def addPrettyPrinters():
    pp = gdb.printing.RegexpCollectionPrettyPrinter('mimiker')
    pp.add_printer('mtx', 'mtx', mutex.Mutex)
    pp.add_printer('thread', 'thread', thread.Thread)
    gdb.printing.register_pretty_printer(gdb.current_objfile(), pp)

addPrettyPrinters()
ktrace.Ktrace()
thread.Kthread()
kdump.Kdump()
klog.Klog()
