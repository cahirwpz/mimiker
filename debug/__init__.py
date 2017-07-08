import gdb
import ktrace
import kdump
import klog
import sync
import thread


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
kdump.Kdump()
klog.Klog()
