from .simulators import *
from .outputs import *
from .debuggers import *

# The items on these lists should be ordered from the most desirable. The first
# available item will become the default option.
SIMULATORS = [OVPsim(),
              QEMU()]
OUTPUTS = [XTermOutput(),
           StdIOOutput(),
           ServerOutput()]


DEBUGGERS = {
    'gdb': '%(triplet)s-gdb %(kernel)s',
    'cgdb': 'cgdb -d %(triplet)s-gdb %(kernel)s',
    'ddd': 'ddd --debugger %(triplet)s-gdb %(kernel)s',
    'gdbtui': '%(triplet)s-gdb -tui %(kernel)s',
    'emacs': 'emacsclient -c -e \'(gdb "%(triplet)s-gdb -i=mi %(kernel)s")\''
}

DEBUGGERS = [NoDebugger(),
             GDBWrapper('cgdb', 'cgdb', 'cgdb -d %(triplet)s-gdb %(kernel)s'),
             GDBWrapper('ddd', 'ddd', 'ddd --debugger %(triplet)s-gdb %(kernel)s'),
             GDBWrapper('emacs', 'emacs', 'emacsclient -c -e \'(gdb "%(triplet)s-gdb -i=mi %(kernel)s")\''),
             GDBWrapper('gdbtui', 'gdbtui', '%(triplet)s-gdb -tui %(kernel)s'),
             GDBWrapper('gdb', '', '%(triplet)s-gdb %(kernel)s'),]

__all__ = ["common",
           "SIMULATORS",
           "OUTPUTS",
           "DEBUGGERS",
           "UART_PORT"]
