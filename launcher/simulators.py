import os
from .common import *


class QEMU(Launchable):

    def __init__(self):
        Launchable.__init__(self, 'qemu')

    def probe(self):
        self.cmd = shutil.which('qemu-mimiker-mipsel')
        return self.cmd is not None

    def configure(self, **kwargs):
        port = kwargs['uart_port']
        self.options = ['-nodefaults',
                        '-device', 'VGA',
                        '-machine', 'malta',
                        '-cpu', '24Kf',
                        '-icount', 'shift=3,sleep=on',
                        '-kernel', kwargs['kernel'],
                        '-append', kwargs['args'],
                        '-gdb', 'tcp::%d' % kwargs['gdb_port'],
                        '-serial', 'none',
                        '-serial', 'null',
                        '-serial', 'null',
                        '-serial', 'tcp:127.0.0.1:%d,server,wait' % port]
        if kwargs['debug']:
            self.options += ['-S']

        if not kwargs['graphics']:
            self.options += ['-display', 'none']

SIMULATORS = [QEMU()]


def find_available():
    return common.find_available(SIMULATORS)
