import gdb
import thread
import physmem
import klog
import pmem
import utils


class Kdump(gdb.Command, utils.OneArgAutoCompleteMixin):
    """
    kdump is command for examining kernel state.
    Currently supported commands:
    kdump threads - lists information about threads in kernel
    """
    def __init__(self):
        super(Kdump, self).__init__("kdump", gdb.COMMAND_USER)
        # classes instead of functions in case we decide to store
        # information about structures in the debugger itself later on
        self.structure = {
            'threads': thread.KernelThreads(),
            'segments': physmem.KernelSegments(),
            'free_pages': physmem.KernelFreePages(),
            'tlb': pmem.TLB(),
        }

    def invoke(self, args, from_tty):
        if len(args) < 1:
            raise(gdb.GdbError('Usage: kdump [structure]. Structures: {}.'
                               .format(self.structure.keys())))
        if args not in self.structure:
            raise gdb.GdbError('No such structure - {}.'.format(args))
        self.structure[args].invoke()

    def options(self):
        return self.structure.keys()
