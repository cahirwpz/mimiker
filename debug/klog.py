import gdb
import os.path

from .cmd import print_exception
from .utils import GdbStructMeta, TextTable, global_var, enum, cstr, relpath


class TimeVal(metaclass=GdbStructMeta):
    __ctype__ = 'struct timeval'
    __cast__ = {'tv_sec': int, 'tv_usec': int}

    def as_float(self):
        return float(self.tv_sec) + float(self.tv_usec) * 1e-6

    def __str__(self):
        return 'timeval{%.6f}' % self.as_float()


class LogEntry(metaclass=GdbStructMeta):
    __ctype__ = 'struct klog_entry'
    __cast__ = {'kl_tid': int, 'kl_timestamp': TimeVal, 'kl_file': cstr,
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
    __cast__ = {'verbose': bool}

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


class Klog(gdb.Command):
    """TODO: documentation"""

    def __init__(self):
        super().__init__('klog', gdb.COMMAND_USER)

    @print_exception
    def invoke(self, args, from_tty):
        klog = LogBuffer(global_var('klog'))
        self.dump_info(klog)
        self.dump_messages(klog)

    def dump_info(self, klog):
        table = TextTable(types='tti', align='rrr')
        table.header(['Mask', 'Verbose', 'Messages'])
        table.add_row([hex(klog.mask), klog.verbose, len(klog)])
        print(table)

    def dump_messages(self, klog):
        table = TextTable(types='', align='rrlll')
        table.header(['Time', 'Id', 'Source', 'System', 'Message'])
        table.set_precision(6)
        for entry in klog:
            table.add_row([entry.kl_timestamp.as_float(), entry.kl_tid,
                           entry.source, entry.kl_origin, entry.format_msg()])
        print(table)
