import gdb
import sys
import os
sys.path.append(os.getcwd() + '/gdbscripts')
import threads

class Kdump(gdb.Command):
    def __init__(self):
        super(Kdump, self).__init__("kdump", gdb.COMMAND_USER)
        # classes instread of functions in case we decided to store
        # some information about structres in the debugger itself later on
        self.structure = {
            'threads': threads.KernelThreads(),
        }

    def invoke(self, args, from_tty):
        if len(args) < 1:
            raise gdb.GdbError("Usage: kdump [structure]. Structures: {}." \
                    .format(self.structure.keys()))
        if args in self.structure:
            self.structure[args].invoke()
        else:
            raise gdb.GdbError("No such structure - {}.".format(args))

    def complete(self, text, word):
        suggestion = []
        args = text.split()
        if len(args) == 0: 
            return self.structure.keys()
        if len(args) >= 2: 
            return []
        for k in self.structure.keys():
            if k.startswith(args[0], 0, len(k) - 1): 
                suggestion.append(k)
        return suggestion

Kdump()
