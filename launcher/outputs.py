from .common import *

class ServerOutput(Launchable):
    # This is a no-op class.
    def __init__(self):
        Launchable.__init__(self, 'server')
    def probe(self):
        return True
    def configure(self, kernel, args="", debug=False, uart_port=8000):
        return

class XTermOutput(Launchable):
    def __init__(self):
        Launchable.__init__(self, 'xterm', needs_new_session=True)
    def probe(self):
        self.cmd = shutil.which('xterm')
        return self.cmd is not None
    def configure(self, kernel, args="", debug=False, uart_port=8000):
        # The simulator will only open the server after some time has
        # passed. OVPsim needs as much as 1 second. To minimize the delay, keep
        # reconnecting until success.
        self.options = ['-title', 'UART2', '-e', 'socat STDIO tcp:localhost:%d,retry,forever' % uart_port]

class StdIOOutput(Launchable):
    def __init__(self):
        Launchable.__init__(self, 'stdio')
    def probe(self):
        return True
    def configure(self, kernel, args="", debug=False, uart_port=8000):
        # The simulator will only open the server after some time has
        # passed. OVPsim needs as much as 1 second. To minimize the delay, keep
        # reconnecting until success.
        self.cmd = 'socat'
        self.options = ['STDIO', 'tcp:localhost:%d,retry,forever' % uart_port]
