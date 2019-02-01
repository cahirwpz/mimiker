import gdb

from .struct import GdbStructMeta
from .utils import TextTable, cast
from .cmd import UserCommand, CommandDispatcher


PAGESIZE = 0x1000


class TLBHi():
    def __init__(self, val):
        self.val = cast(val, 'unsigned long')

    @property
    def vpn(self):
        return self.val & 0xffffe000

    @property
    def asid(self):
        return self.val & 0x000000ff


class TLBLo():
    def __init__(self, val):
        self.val = cast(val, 'unsigned long')

    @property
    def globl(self):
        return bool(self.val & 1)

    @property
    def valid(self):
        return bool(self.val & 2)

    @property
    def dirty(self):
        return bool(self.val & 4)

    @property
    def ppn(self):
        return (self.val & 0x03ffffc0) << 6

    def __str__(self):
        return '%08x %c%c' % (self.ppn, '-D'[self.dirty], '-G'[self.globl])


class TLBEntry(metaclass=GdbStructMeta):
    __ctype__ = 'tlbentry_t'
    __cast__ = {'hi': TLBHi, 'lo0': TLBLo, 'lo1': TLBLo}

    def dump(self):
        if not self.lo0.valid and not self.lo1.valid:
            return None
        lo0, lo1 = '-', '-'
        if self.lo0.valid:
            lo0 = '%08x %s' % (self.hi.vpn, self.lo0)
        if self.lo1.valid:
            lo1 = '%08x %s' % (self.hi.vpn + PAGESIZE, self.lo1)
        return ['%02x' % self.hi.asid, lo0, lo1]


class TLB(UserCommand):
    """List Translation Lookaside Buffer entries"""

    def __init__(self):
        super().__init__('tlb')

    def __call__(self, args):
        table = TextTable(align='rrll')
        table.header(["Index", "ASID", "PFN0", "PFN1"])
        for idx in range(TLB.size()):
            row = TLB.read(idx).dump()
            if row is None:
                continue
            table.add_row([str(idx)] + row)
        print(table)

    @staticmethod
    def read(idx):
        gdb.parse_and_eval('_gdb_tlb_read_index(%d)' % idx)
        return TLBEntry(gdb.parse_and_eval('_gdb_tlb_entry'))

    @staticmethod
    def size():
        return int(gdb.parse_and_eval('_gdb_tlb_size()'))


class Cpu(CommandDispatcher):
    """Examine processor priviliged resources."""

    def __init__(self):
        super().__init__('cpu', [TLB()])
