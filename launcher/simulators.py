import os
from .common import *


class OVPsim(Launchable):

    def __init__(self):
        Launchable.__init__(self, 'ovpsim')

    def probe(self):
        try:
            OVPSIM_VENDOR = 'mips.ovpworld.org'
            OVPSIM_PLATFORM = 'MipsMalta/1.0'
            IMPERAS_VLNV = os.environ['IMPERAS_VLNV']
            IMPERAS_ARCH = os.environ['IMPERAS_ARCH']

            self.cmd = os.path.join(
                IMPERAS_VLNV, OVPSIM_VENDOR, 'platform', OVPSIM_PLATFORM,
                'platform.%s.exe' % IMPERAS_ARCH)
            return True
        except KeyError:
            return False

    def configure(self, **kwargs):
        try:
            os.remove('uartCBUS.log')
            os.remove('uartTTY0.log')
            os.remove('uartTTY1.log')
        except OSError:
            pass

        OVPSIM_OVERRIDES = {
            'mipsle1/vectoredinterrupt': 1,
            'mipsle1/srsctlHSS': 1,
            'rtc/timefromhost': 1,
            'uartCBUS/console': 0,
            'uartCBUS/portnum': kwargs['uart_port'],
            'command': '"' + kwargs['args'] + '"'
        }

        self.options = ['--ramdisk', 'initrd.cpio',
                        '--wallclock',
                        '--kernel', kwargs['kernel']]

        if not kwargs['graphics']:
            self.options += ['--nographics']

        if kwargs['debug']:
            self.options += ['--port', str(kwargs['gdb_port'])]

        for item in OVPSIM_OVERRIDES.items():
            self.options += ['--override', '%s=%s' % item]


class QEMU(Launchable):

    def __init__(self):
        Launchable.__init__(self, 'qemu')

    def probe(self):
        self.cmd = shutil.which('qemu-system-mipsel')
        return self.cmd is not None

    def configure(self, **kwargs):
        port = kwargs['uart_port']
        self.options = ['-nodefaults',
                        '-device', 'VGA',
                        '-machine', 'malta',
                        '-cpu', '24Kf',
                        '-icount', 'shift=7',
                        '-kernel', kwargs['kernel'],
                        '-append', kwargs['args'],
                        '-gdb', 'tcp::%d' % kwargs['gdb_port'],
                        '-initrd', 'initrd.cpio',
                        '-serial', 'none',
                        '-serial', 'null',
                        '-serial', 'null',
                        '-serial', 'tcp:127.0.0.1:%d,server,wait' % port]
        if kwargs['debug']:
            self.options += ['-S']

        if not kwargs['graphics']:
            self.options += ['-display', 'none']

SIMULATORS = [OVPsim(), QEMU()]


def find_available():
    return common.find_available(SIMULATORS)
