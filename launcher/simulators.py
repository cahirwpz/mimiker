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

    def configure(self, kernel, args="", debug=False, uart_port=8000):
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
            'uartCBUS/portnum': uart_port,
            'command': '"' + args + '"'
        }

        self.options = ['--ramdisk', 'initrd.cpio',
                        '--nographics',
                        '--wallclock',
                        '--kernel', kernel]

        if debug:
            self.options += ['--port', '1234']

        for item in OVPSIM_OVERRIDES.items():
            self.options += ['--override', '%s=%s' % item]


class QEMU(Launchable):

    def __init__(self):
        Launchable.__init__(self, 'qemu')

    def probe(self):
        self.cmd = shutil.which('qemu-system-mipsel')
        return self.cmd is not None

    def configure(self, kernel, args="", debug=False, uart_port=8000):
        self.options = ['-nographic',
                        '-nodefaults',
                        '-machine', 'malta',
                        '-cpu', '24Kf',
                        '-kernel', kernel,
                        '-append', args,
                        '-gdb', 'tcp::1234',
                        '-initrd', 'initrd.cpio',
                        '-serial', 'none',
                        '-serial', 'null',
                        '-serial', 'null',
                        '-serial', 'tcp:127.0.0.1:%d,server,wait' % uart_port]
        if debug:
            self.options += ['-S']


SIMULATORS = [OVPsim(),
              QEMU()]
