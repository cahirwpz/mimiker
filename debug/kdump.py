import gdb
import sys
import os
sys.path.append(os.path.join(os.getcwd(), 'debug'))
import threads
import physmem


class Kdump(gdb.Command):
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
            'threads': threads.KernelThreads(),
            'segments': physmem.KernelSegments(),
            'free_pages': physmem.KernelFreePages(),
            'tlb': physmem.TLB()
        }

    def invoke(self, args, from_tty):
        if len(args) < 1:
            raise(gdb.GdbError('Usage: kdump [structure]. Structures: {}.'
                               .format(self.structure.keys())))
        if args not in self.structure:
            raise gdb.GdbError('No such structure - {}.'.format(args))
        self.structure[args].invoke()

    def complete(self, text, word):
        args = text.split()
        if len(args) == 0:
            return self.structure.keys()
        if len(args) >= 2:
            return []
        suggestions = []
        for k in self.structure.keys():
            if k.startswith(args[0], 0, len(k) - 1):
                suggestions.append(k)
        return suggestions

Kdump()
