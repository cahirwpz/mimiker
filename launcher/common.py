import subprocess
import itertools
import shutil
import signal

UART_PORT = 8000
TRIPLET = 'mipsel-unknown-elf'

class Launchable:
    def __init__(self, name, needs_new_session=False):
        self.name = name
        self.process = None
        self.cmd = None
        self.needs_new_session = needs_new_session
    def probe(self):
        raise NotImplementedError('This launchable is not probable')

    def configure(self, kernel, args="", debug=False, uart_port=8000):
        raise NotImplementedError('This launchable is not configurable')
        
    def start(self):
        if self.cmd is None:
            return
        print(self.needs_new_session)
        self.process = subprocess.Popen([self.cmd] + self.options,
                                        start_new_session=self.needs_new_session)

    # Returns true iff the process terminated
    def wait(self, timeout=None):
        if self.process is None:
            return
        self.process.wait(timeout)
        return True
        
    def stop(self):
        if self.cmd is None:
            return
        # Give a chance to exit gracefuly.
        self.process.send_signal(signal.SIGTERM)
        try:
            self.process.wait(0.2)
        except subprocess.TimeoutExpired:
            self.process.send_signal(signal.SIGKILL)
        self.process = None

        
def find_available(l):
    result, default = {}, None
    for item in reversed(l):
        if item.probe():
            result[item.name] = item
            default = item.name
    return (result, default)

def wait_any(launchables):
    for l in itertools.cycle(launchables):
        try:
            if l.wait(0.2):
                print(l.name + ' terminated')
                return
        except subprocess.TimeoutExpired:
            pass
        except KeyboardInterrupt:
            return

def find_toolchain():
    if not shutil.which(TRIPLET + '-gcc'):
        raise SystemExit('No cross compiler found!')
