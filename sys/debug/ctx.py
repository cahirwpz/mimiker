import gdb
from collections import OrderedDict

from .utils import TextTable


class Context():
    names = ['at', 'v0', 'v1', 'a0', 'a1', 'a2', 'a3',
             't0', 't1', 't2', 't3', 't4', 't5', 't6', 't7',
             's0', 's1', 's2', 's3', 's4', 's5', 's6', 's7',
             't8', 't9', 'gp', 'sp', 's8', 'ra',
             'lo', 'hi', 'pc', 'sr', 'bad', 'cause']
    aliases = {'bad': 'badvaddr', 's8': 'fp'}

    def __init__(self):
        self.regs = OrderedDict()
        for name in self.names:
            self.regs[name] = 0

    @classmethod
    def load(cls, frame):
        for name in cls.names:
            alias = cls.aliases.get(name, name)
            try:
                gdb.execute('set $%s = %d' % (name, frame[alias]))
            except Exception:
                pass

    def save(self):
        for name in self.names:
            val = gdb.parse_and_eval('$' + name)
            self.regs[name] = int(val.cast(gdb.lookup_type('register_t')))

    def restore(self):
        for name, val in self.regs.items():
            gdb.execute('set $%s = %d' % (name, val))

    def dump(self):
        table = TextTable(align='rl')
        table.add_rows([[name, str(val)] for name, val in self.regs.items()])
        print(table)
