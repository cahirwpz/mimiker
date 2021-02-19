import gdb
from collections import OrderedDict

from .utils import TextTable


class Context():
    names = {
        'mips': ['at', 'v0', 'v1', 'a0', 'a1', 'a2', 'a3',
                't0', 't1', 't2', 't3', 't4', 't5', 't6', 't7',
                's0', 's1', 's2', 's3', 's4', 's5', 's6', 's7',
                't8', 't9', 'k0', 'k1', 'gp', 'sp', 's8', 'ra',
                'lo', 'hi', 'cause', 'pc', 'sr', 'bad'],
        'aarch64': ['x0', 'x1', 'x2', 'x3', 'x4', 'x5', 'x6', 'x7',
                    'x8', 'x9', 'x10', 'x11', 'x12', 'x13', 'x14', 'x15',
                    'x16', 'x17', 'x18', 'x19', 'x20', 'x21', 'x22', 'x23',
                    'x24', 'x25', 'x26', 'x27', 'x28', 'x29', 'lr', 'sp', 'pc']
    }

    def __init__(self):
        if gdb.selected_frame().architecture().name() == 'mips:isa32r2':
            self.arch = 'mips'
            self.reg_size = 32
        if gdb.selected_frame().architecture().name() == 'aarch64':
            self.arch = 'aarch64'
            self.reg_size = 64
        self.regs = OrderedDict()
        for name in self.names[self.arch]:
            self.regs[name] = 0

    @classmethod
    def from_kctx(cls, kctx):
        c = Context()
        for i, name in enumerate(cls.names[c.arch]):
            c.regs[name] = int(kctx['__gregs'][i])
        # `kctx` points to the context on the kernel stack.
        # The program counter in that context is actually the return address
        # from ctx_switch(), pointing to somewhere in sched_switch().
        # Therefore, we need to skip the stack frame of ctx_switch() for gdb
        # to print a proper backtrace. This is why we add the size
        # of `*td_kctx` to `kctx`.
        c.regs['sp'] = int(kctx) + kctx.type.target().sizeof
        return c

    @classmethod
    def load(cls, ctx):
        for name, val in ctx.regs.items():
            gdb.execute('set $%s = %d' % (name, val))

    @classmethod
    def current(cls):
        c = Context()
        for name in cls.names[c.arch]:
            val = gdb.parse_and_eval('$' + name)
            c.regs[name] = int(val.cast(gdb.lookup_type('__greg_t')))
        return c

    def dump(self):
        table = TextTable(align='rl')
        if self.reg_size == 32:
            table.add_rows([[name, '0x%08x' % (val & 0xffffffff)]
                            for name, val in self.regs.items()])
        elif self.reg_size == 64:
            table.add_rows([[name, '0x%16x' % (val & 0xffffffffffffffff)]
                            for name, val in self.regs.items()])
        print(table)
