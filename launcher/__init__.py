import itertools
import os
import shlex
import shutil
import signal
import subprocess

FIRST_UID = 1000


# Reserve 10 ports for each user starting from 24000
def gdb_port():
    return 24000 + (os.getuid() - FIRST_UID) * 10


def uart_port(num):
    assert num >= 0 and num < 9
    return gdb_port() + 1 + num


CONFIG = {
    'board': 'malta',
    'config': {
        'debug': False,
        'graphics': False,
        'kernel': 'sys/mimiker.elf',
        'initrd': 'initrd.cpio',
        'args': []
    },
    'qemu': {
        'options': [
            '-nodefaults',
            '-icount', 'shift=3,sleep=on',
            '-kernel', '{kernel}',
            '-initrd', '{initrd}',
            '-gdb', 'tcp:127.0.0.1:{},server,wait'.format(gdb_port()),
            '-serial', 'none'],
        'board': {
            'malta': {
                'binary': 'qemu-mimiker-mipsel',
                'options': [
                    '-device', 'VGA',
                    '-machine', 'malta',
                    '-cpu', '24Kf'],
                'uarts': [
                    ('/dev/tty1', uart_port(0)),
                    ('/dev/tty2', uart_port(1)),
                    ('/dev/cons', uart_port(2))
                ]
            },
            'rpi3': {
                'binary': 'qemu-mimiker-aarch64',
                'options': [
                    '-machine', 'raspi3',
                    '-smp', '4',
                    '-cpu', 'cortex-a53'],
                'uarts': [
                    ('/dev/cons', uart_port(0))
                ]
            }
        }
    },
    'gdb': {
        'pre-options': [
            '-n',
            '-ex=set confirm no',
            '-iex=set auto-load safe-path {}/'.format(os.getcwd()),
            '-ex=set tcp connect-timeout 30',
            '-ex=target remote localhost:{}'.format(gdb_port()),
            '--silent',
        ],
        'extra-options': [],
        'post-options': [
            '-ex=set confirm yes',
            '-ex=source .gdbinit',
            '-ex=continue',
            '{kernel}'
        ],
        'board': {
            'malta': {
                'binary': 'mipsel-mimiker-elf-gdb'
            },
            'rpi3': {
                'binary': 'aarch64-mimiker-elf-gdb'
            }
        }
    }
}


def split(name):
    return name.format(BOARD='board.' + CONFIG['board']).split('.')


def getvar(name, val=CONFIG):
    for f in split(name):
        val = val[f]
    return val


def setvar(name, val, config=CONFIG):
    fs = split(name)
    while len(fs) > 1:
        config = config[fs.pop(0)]
    config[fs.pop(0)] = val


def getopts(*names):
    opts = itertools.chain.from_iterable([getvar(name) for name in names])
    return [opt.format(**getvar('config')) for opt in opts]


class Launchable():
    def __init__(self, name, cmd):
        self.name = name
        self.cmd = cmd
        self.window = None
        self.options = []

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
        super().__init__('qemu', getvar('qemu.{BOARD}.binary'))

        self.options = getopts('qemu.options', 'qemu.{BOARD}.options')
        for _, port in getvar('qemu.{BOARD}.uarts'):
            self.options += ['-serial', f'tcp:127.0.0.1:{port},server,wait']

        if getvar('config.args'):
            self.options += ['-append', ' '.join(getvar('config.args'))]
        if getvar('config.debug'):
            self.options += ['-S']
        if not getvar('config.graphics'):
            self.options += ['-display', 'none']


class GDB(Launchable):
    def __init__(self, name=None, cmd=None):
        super().__init__(name or 'gdb', cmd or getvar('gdb.{BOARD}.binary'))
        # gdbtui & cgdb output is garbled if there is no delay
        self.cmd = 'sleep 0.25 && ' + self.cmd

        if self.name == 'gdb':
            self.options += ['-ex=set prompt \033[35;1m(gdb) \033[0m']
        self.options += getopts(
                'gdb.pre-options', 'gdb.extra-options', 'gdb.post-options')


class GDBTUI(GDB):
    def __init__(self):
        super().__init__('gdbtui')
        self.options = ['-tui']


class CGDB(GDB):
    def __init__(self):
        super().__init__('cgdb', 'cgdb')
        self.options = ['-d', getvar('gdb.{BOARD}.binary')]


class SOCAT(Launchable):
    def __init__(self, name, tcp_port):
        super().__init__(name, 'socat')
        # The simulator will only open the server after some time has
        # passed.  To minimize the delay, keep reconnecting until success.
        self.options = ['STDIO', f'tcp:localhost:{tcp_port},retry,forever']


Debuggers = {'gdb': GDB, 'gdbtui': GDBTUI, 'cgdb': CGDB}
__all__ = ['Launchable', 'QEMU', 'GDB', 'CGDB', 'GDBTUI', 'SOCAT',
           'Debuggers', 'gdb_port', 'uart_port', 'getvar', 'setvar']
