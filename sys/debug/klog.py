import gdb
import os.path

from .cmd import SimpleCommand
from .struct import GdbStructMeta, enum, cstr, BinTime
from .utils import TextTable, global_var, relpath


class LogEntry(metaclass=GdbStructMeta):
    __ctype__ = 'struct klog_entry'
    __cast__ = {'kl_tid': int, 'kl_timestamp': BinTime, 'kl_file': cstr,
                'kl_format': cstr, 'kl_line': int, 'kl_origin': enum}

    @property
    def source(self):
        return '{}:{}'.format(relpath(self.kl_file), self.kl_line)

    def format_msg(self):
        msg = self.kl_format.replace('"', '\\"')
        # If there is % escaped it is not parameter.
        nparams = msg.count('%') - 2 * msg.count('%%')
        params = [str(self.kl_params[i]) for i in range(nparams)]
        printf = 'printf "%s", %s' % (msg, ', '.join(params))
        try:
            # Using gdb printf so we don't need to dereference addresses.
            return gdb.execute(printf, to_string=True)
        except Exception:
            # Do not format the message, because something went wrong.
            return printf


class LogBuffer(metaclass=GdbStructMeta):
    __ctype__ = 'struct klog'
    __cast__ = {'first': int, 'last': int}

    @property
    def size(self):
        return int(self.array.type.range()[1]) + 1

    def __iter__(self):
        first, last = self.first, self.last
        while first != last:
            yield LogEntry(self.array[first])
            first = (first + 1) % self.size

    def __len__(self):
        n = self.last - self.first
        if n < 0:
            n += self.size
        return n


class Klog(SimpleCommand):
    """Display kernel log buffer."""

    def __init__(self):
        super().__init__('klog')

    def __call__(self, args):
        klog = LogBuffer(global_var('klog'))
        self.dump_info(klog)
        self.dump_messages(klog)

    def dump_info(self, klog):
        table = TextTable(types='ti', align='rr')
        table.header(['Mask', 'Messages'])
        table.add_row([hex(klog.mask), len(klog)])
        print(table)

    def dump_messages(self, klog):
        table = TextTable(types='', align='rrlll')
        table.header(['Time', 'Id', 'Source', 'System', 'Message'])
        table.set_precision(6)
        for entry in klog:
            table.add_row([entry.kl_timestamp.as_float(), entry.kl_tid,
                           entry.source, entry.kl_origin, entry.format_msg()])
        print(table)
