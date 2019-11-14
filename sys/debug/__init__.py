import gdb

from .cpu import Cpu
from .kdump import Kdump
from .klog import Klog
from .ktrace import Ktrace
from .proc import Kprocess, Process, CurrentProcess
from .struct import TimeVal
from .sync import CondVar, Mutex
from .thread import Kthread, Thread, CurrentThread

#from .event_handlers import stop_handler, UserReturnBP
from .event_handlers import get_stop_handler


def addPrettyPrinters():
    pp = gdb.printing.RegexpCollectionPrettyPrinter('mimiker')
    pp.add_printer('mtx', 'mtx', Mutex)
    pp.add_printer('condvar', 'condvar', CondVar)
    pp.add_printer('thread', 'thread', Thread)
    pp.add_printer('process', 'process', Process)
    pp.add_printer('timeval', 'timeval', TimeVal)
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

elf_dict = {}
with open('elf_files', 'r') as f:
    for line in f:
        mimiker, host = line.strip().split(':', 1)
        elf_dict[mimiker] = host

# Events
gdb.events.stop.connect(get_stop_handler(elf_dict))
