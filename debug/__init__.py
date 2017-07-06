import gdb
import ktrace
import kdump
import mutex
import thread


def addPrettyPrinters():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("mimiker")
    pp.add_printer('mtx', '^mtx$', mutex.PrettyPrinter)
    pp.add_printer('thread', '^thread$', thread.PrettyPrinter)
    gdb.printing.register_pretty_printer(gdb.current_objfile(), pp)

addPrettyPrinters()
ktrace.Ktrace()
thread.Kthread()
kdump.Kdump()
