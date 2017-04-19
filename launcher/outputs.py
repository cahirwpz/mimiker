from .common import *


class ServerOutput(Launchable):
    # This is a no-op class.

    def __init__(self):
        Launchable.__init__(self, 'server')

    def probe(self):
        return True

    def configure(self, **kwargs):
        pass


class XTermOutput(Launchable):

    def __init__(self):
        Launchable.__init__(self, 'xterm')

    def probe(self):
        self.cmd = shutil.which('xterm')
        return self.cmd is not None

    def configure(self, **kwargs):
        # The simulator will only open the server after some time has
        # passed. OVPsim needs as much as 1 second. To minimize the delay, keep
        # reconnecting until success.
        self.options = ['-title', 'UART2', '-e',
                        'socat STDIO tcp:localhost:%d,retry,forever' % kwargs['uart_port']]


class StdIOOutput(Launchable):

    def __init__(self):
        Launchable.__init__(self, 'stdio')

    def probe(self):
        return True

    def configure(self, **kwargs):
        # The simulator will only open the server after some time has
        # passed. OVPsim needs as much as 1 second. To minimize the delay, keep
        # reconnecting until success.
        self.cmd = 'socat'
        self.options = ['STDIO', 'tcp:localhost:%d,retry,forever' % kwargs['uart_port']]


OUTPUTS = [XTermOutput(),
           StdIOOutput(),
           ServerOutput()]
