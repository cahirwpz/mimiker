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
        port = kwargs['uart_port']
        # The simulator will only open the server after some time has
        # passed. OVPsim needs as much as 1 second. To minimize the delay, keep
        # reconnecting until success.
        self.options = ['-title', 'UART2', '-e',
                        'socat STDIO tcp:localhost:%d,retry,forever' % port]


class UrxvtOutput(Launchable):

    def __init__(self):
        Launchable.__init__(self, 'urxvt')

    def probe(self):
        self.cmd = shutil.which('urxvt')
        return self.cmd is not None

    def configure(self, **kwargs):
        port = kwargs['uart_port']
        # The simulator will only open the server after some time has
        # passed. OVPsim needs as much as 1 second. To minimize the delay, keep
        # reconnecting until success.
        self.options = ['-title', 'UART2', '-e', 'sh', '-c',
                        'socat STDIO tcp:localhost:%d,retry,forever' % port]


class StdIOOutput(Launchable):

    def __init__(self):
        Launchable.__init__(self, 'stdio')

    def probe(self):
        return True

    def configure(self, **kwargs):
        port = kwargs['uart_port']
        # The simulator will only open the server after some time has
        # passed. OVPsim needs as much as 1 second. To minimize the delay, keep
        # reconnecting until success.
        self.cmd = 'socat'
        self.options = ['STDIO', 'tcp:localhost:%d,retry,forever' % port]


OUTPUTS = [UrxvtOutput(),
           XTermOutput(),
           StdIOOutput(),
           ServerOutput()]
