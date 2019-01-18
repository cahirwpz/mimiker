import gdb

from .ktrace import Ktrace
from .kdump import Kdump
from .klog import Klog, TimeVal
from .cpu import Cpu
from .thread import Kthread, Thread
from .sync import CondVar, Mutex


def addPrettyPrinters():
    pp = gdb.printing.RegexpCollectionPrettyPrinter('mimiker')
    pp.add_printer('mtx', 'mtx', Mutex)
    pp.add_printer('condvar', 'condvar', CondVar)
    pp.add_printer('thread', 'thread', Thread)
    pp.add_printer('timeval', 'timeval', TimeVal)
    gdb.printing.register_pretty_printer(gdb.current_objfile(), pp)

addPrettyPrinters()
Ktrace()
Kthread()
Cpu()
Kdump()
Klog()
