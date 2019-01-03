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

    def configure(self, **kwargs):
        # TODO: Maybe we don't need a way of configuring the tripet.
        cmd = shlex.split(self.cmdstring % {'kernel': kwargs['kernel'],
                                            'gdb': TRIPLET + '-gdb',
                                            'opts': '-ex=continue --silent'})
        self.cmd, self.options = cmd[0], cmd[1:]

    def stop(self):
        Launchable.stop(self)
        # This is necessary because cgdb is irresponsible, and does not clean
        # up terminal configuration (e.g. disabled keystroke echo)
        os.system("stty sane")


class NoDebugger(Launchable):

    def __init__(self):
        Launchable.__init__(self, 'none')

    def probe(self):
        return True

    def configure(self, **kwargs):
        pass


DEBUGGERS = [
    NoDebugger(),
    GDBWrapper('cgdb', 'cgdb', 'cgdb -d %(gdb)s %(opts)s %(kernel)s', True),
    GDBWrapper('ddd', 'ddd', 'ddd --debugger %(gdb)s %(opts)s %(kernel)s'),
    GDBWrapper('emacs', 'emacs', 'emacsclient -c -e'
               '\'(gdb "%(gdb)s -i=mi %(opts)s %(kernel)s")\''),
    GDBWrapper('gdbtui', 'gdbtui', '%(gdb)s %(opts)s -tui %(kernel)s', True),
    GDBWrapper('gdb', '', '%(gdb)s %(opts)s %(kernel)s', True),
]
