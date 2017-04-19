import subprocess
import itertools
import shutil
import signal
import os

TRIPLET = 'mipsel-unknown-elf'


def set_as_tty_foreground():
    # Create a new process group just for us
    os.setpgrp()
    # Becoming terminal foreground sends SIGTTOU which normally kills the
    # process. Ignore SIGTTOU.
    signal.signal(signal.SIGTTOU, signal.SIG_IGN)
    # Prepare a terminal device
    tty = os.open('/dev/tty', os.O_RDWR)
    # Become terminal foreground
    os.tcsetpgrp(tty, os.getpgrp())


class Launchable:

    def __init__(self, name, needs_foreground=False):
        self.name = name
        self.process = None
        self.cmd = None
        self.needs_foreground = needs_foreground

    def probe(self):
        raise NotImplementedError('This launchable is not probable')

    def configure(self, **kwargs):
        raise NotImplementedError('This launchable is not configurable')

    def start(self):
        if self.cmd is None:
            return
        on_spawn = set_as_tty_foreground if self.needs_foreground else None
        self.process = subprocess.Popen([self.cmd] + self.options,
                                        start_new_session=False,
                                        preexec_fn=on_spawn)

    # Returns true iff the process terminated
    def wait(self, timeout=None):
        if self.process is None:
            return False
        self.process.wait(timeout)
        return True

    def stop(self):
        if self.process is None:
            return
        # Give a chance to exit gracefuly.
        self.process.send_signal(signal.SIGTERM)
        try:
            self.process.wait(0.2)
        except subprocess.TimeoutExpired:
            self.process.send_signal(signal.SIGKILL)
        self.process = None

    def interrupt(self):
        if self.process is not None:
            self.process.send_signal(signal.SIGINT)

    @staticmethod
    # The items on these lists should be ordered from the most desirable. The first
    # available item will become the default option.
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
                # print(l.name + ' terminated')
                return
        except subprocess.TimeoutExpired:
            continue


def find_toolchain():
    if not shutil.which(TRIPLET + '-gcc'):
        raise SystemExit('No cross compiler found!')
