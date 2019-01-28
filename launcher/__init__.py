import itertools
import os
import shlex
import shutil
import signal
import subprocess


TARGET = 'mipsel'
FIRST_UID = 1000


# Reserve 10 ports for each user starting from 24000
def gdb_port():
    return 24000 + (os.getuid() - FIRST_UID) * 10


def uart_port(num):
    assert num >= 0 and num < 9
    return gdb_port() + 1 + num


class Launchable():
    def __init__(self, name, cmd):
        self.name = name
        self.cmd = cmd
        self.window = None
        self.options = []

    def configure(self, *args, **kwargs):
        raise NotImplementedError

    def start(self, session):
        cmd = ' '.join([self.cmd] + list(map(shlex.quote, self.options)))
        self.window = session.new_window(
            attach=False, window_name=self.name, window_shell=cmd)

    def run(self):
        self.process = subprocess.Popen([self.cmd] + self.options,
                                        start_new_session=False)

    # Returns true iff the process terminated
    def wait(self, timeout=None):
        if self.process is None:
            return False
        # Throws exception on timeout
        self.process.wait(timeout)
        self.process = None
        return True

    def stop(self):
        if self.process is None:
            return
        try:
            # Give it a chance to exit gracefuly.
            self.process.send_signal(signal.SIGTERM)
            try:
                self.process.wait(0.2)
            except subprocess.TimeoutExpired:
                self.process.send_signal(signal.SIGKILL)
        except ProcessLookupError:
            # Process already quit.
            pass
        self.process = None

    def interrupt(self):
        if self.process is not None:
            self.process.send_signal(signal.SIGINT)

    @staticmethod
    def wait_any(launchables):
        for l in itertools.cycle(launchables):
            try:
                if l.wait(0.2):
                    break
            except subprocess.TimeoutExpired:
                continue


class QEMU(Launchable):
    def __init__(self):
        super().__init__('qemu', shutil.which('qemu-mimiker-' + TARGET))

    def configure(self, debug=False, graphics=False, kernel='', args=''):
        self.options = [
            '-nodefaults',
            '-device', 'VGA',
            '-machine', 'malta',
            '-cpu', '24Kf',
            '-icount', 'shift=3,sleep=on',
            '-kernel', kernel,
            '-gdb', 'tcp:127.0.0.1:{},server,wait'.format(gdb_port()),
            '-serial', 'none',
            '-serial', 'tcp:127.0.0.1:{},server,wait'.format(uart_port(0)),
            '-serial', 'tcp:127.0.0.1:{},server,wait'.format(uart_port(1)),
            '-serial', 'tcp:127.0.0.1:{},server,wait'.format(uart_port(2))]

        if args:
            self.options += ['-append', ' '.join(args)]

        if debug:
            self.options += ['-S']
        if not graphics:
            self.options += ['-display', 'none']


class GDB(Launchable):
    COMMAND = TARGET + '-mimiker-elf-gdb'

    def __init__(self, name=None, cmd=None):
        super().__init__(name or 'gdb', cmd or GDB.COMMAND)
        # gdbtui & cgdb output is garbled if there is no delay
        self.cmd = 'sleep 0.25 && ' + self.cmd

    def configure(self, kernel=''):
        if self.name == 'gdb':
            self.options += ['-ex=set prompt \033[35;1m(gdb) \033[0m']
        self.options += [
            '-iex=set auto-load safe-path {}/'.format(os.getcwd()),
            '-ex=set tcp connect-timeout 30',
            '-ex=target remote localhost:{}'.format(gdb_port()),
            '-ex=continue',
            '--silent',
            kernel]


class GDBTUI(GDB):
    def __init__(self):
        super().__init__('gdbtui')
        self.options = ['-tui']


class CGDB(GDB):
    def __init__(self):
        super().__init__('cgdb', 'cgdb')
        self.options = ['-d', GDB.COMMAND]


class SOCAT(Launchable):
    def __init__(self, name):
        super().__init__(name, 'socat')

    def configure(self, uart_num):
        port = uart_port(uart_num)
        # The simulator will only open the server after some time has
        # passed.  To minimize the delay, keep reconnecting until success.
        self.options = [
            'STDIO', 'tcp:localhost:{},retry,forever'.format(port)]


Debuggers = {'gdb': GDB, 'gdbtui': GDBTUI, 'cgdb': CGDB}

__all__ = ['Launchable', 'QEMU', 'GDB', 'CGDB', 'GDBTUI', 'SOCAT', 'TARGET',
           'Debuggers', 'gdb_port', 'uart_port']
