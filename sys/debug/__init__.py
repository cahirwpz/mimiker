import gdb

from .cpu import Cpu
from .kdump import Kdump
from .klog import Klog
from .ktrace import Ktrace
from .proc import Kprocess, Process, CurrentProcess
from .struct import TimeSpec
from .sync import CondVar, Mutex
from .thread import Kthread, Thread, CurrentThread
from .events import stop_handler


def addPrettyPrinters():
    pp = gdb.printing.RegexpCollectionPrettyPrinter('mimiker')
    pp.add_printer('mtx', 'mtx', Mutex)
    pp.add_printer('condvar', 'condvar', CondVar)
    pp.add_printer('thread', 'thread', Thread)
    pp.add_printer('process', 'process', Process)
    pp.add_printer('timespec', 'timespec', TimeSpec)
    gdb.printing.register_pretty_printer(gdb.current_objfile(), pp)


addPrettyPrinters()

# Commands
Cpu()
Kdump()
Klog()
Kprocess()
Kthread()
Ktrace()

# Functions
CurrentThread()
CurrentProcess()

# Events
gdb.events.stop.connect(stop_handler)
