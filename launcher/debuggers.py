import shlex
import os
from .common import *

class GDBWrapper(Launchable):
    def __init__(self, name, probe_command, cmdstring):
        Launchable.__init__(self, name, needs_new_session=True)
        self.probe_command = probe_command
        self.cmdstring = cmdstring
    def probe(self):
        return self.probe_command == '' or shutil.which(self.probe_command)
    def configure(self, kernel, args="", debug=False, uart_port=8000):
        # TODO: Maybe we don't need a way of configuring the tripet.
        cmd = shlex.split(self.cmdstring % {'kernel': kernel, 'triplet': TRIPLET})
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
