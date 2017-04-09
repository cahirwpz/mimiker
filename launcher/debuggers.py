import shlex
import os
from .common import *


class GDBWrapper(Launchable):

    def __init__(self, name, probe_command, cmdstring, needs_foreground=False):
        Launchable.__init__(self, name, needs_foreground)
        self.probe_command = probe_command
        self.cmdstring = cmdstring

    def probe(self):
        return self.probe_command == '' or shutil.which(self.probe_command)

    def configure(self, kernel, args="", debug=False, uart_port=8000):
        # TODO: Maybe we don't need a way of configuring the tripet.
        cmd = shlex.split(self.cmdstring %
                          {'kernel': kernel, 'triplet': TRIPLET})
        self.cmd, self.options = cmd[0], cmd[1:]

    def stop(self):
        Launchable.stop(self)
        # This is necessary because cgdb is irresponsible, and does not clean up
        # terminal configuration (e.g. disabled keystroke echo)
        os.system("stty sane")


class NoDebugger(Launchable):
    # No-op.

    def __init__(self):
        Launchable.__init__(self, 'none')

    def probe(self):
        return True

    def configure(self, kernel, args="", debug=False, uart_port=8000):
        return


DEBUGGERS = [
    NoDebugger(),
    GDBWrapper('cgdb', 'cgdb', 'cgdb -d %(triplet)s-gdb %(kernel)s', True),
    GDBWrapper('ddd', 'ddd', 'ddd --debugger %(triplet)s-gdb %(kernel)s'),
    GDBWrapper('emacs', 'emacs',
               'emacsclient -c -e \'(gdb "%(triplet)s-gdb -i=mi %(kernel)s")\''),
    GDBWrapper('gdbtui', 'gdbtui', '%(triplet)s-gdb -tui %(kernel)s', True),
    GDBWrapper('gdb', '', '%(triplet)s-gdb %(kernel)s', True),
]
