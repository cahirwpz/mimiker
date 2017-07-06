import gdb
from ptable import ptable, as_hex


class TimeVal():
    def __init__(self, tv):
        self.sec = int(tv['tv_sec'])
        self.usec = int(tv['tv_usec'])

    def as_float(self):
        return float(self.sec) + float(self.usec) * 10e-6


class LogEntry():
    def __init__(self, entry):
        self.msg = entry['kl_format'].string()
        # If there is % escaped it is not parameter.
        nparams = self.msg.count('%') - 2 * self.msg.count('%%')
        self.params = [entry['kl_params'][i] for i in range(nparams)]
        self.timestamp = TimeVal(entry['kl_timestamp'])
        self.source = entry['kl_file'].string()
        self.line = int(entry['kl_line'])
        self.origin = str(entry['kl_origin'])

    def format_msg(self):
        msg = self.msg.replace('"', '\\"')
        params = ', '.join([str(s) for s in self.params])
        printf = 'printf "%s", %s' % (msg, params)
        try:
            # Using gdb printf so we don't need to dereference addresses.
            return gdb.execute(printf, to_string=True)
        except Exception:
            # Do not format the message, because something went wrong.
            return printf


class LogBuffer():
    def __init__(self):
        klog = gdb.parse_and_eval('klog')
        self.first = int(klog['first'])
        self.last = int(klog['last'])
        self.array = klog['array']
        self.size = int(self.array.type.range()[1]) + 1
        self.mask = int(klog['mask'])
        self.verbose = bool(klog['verbose'])

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


class KernelLog():
    def invoke(self):
        klog = LogBuffer()
        self.dump_info(klog)
        self.dump_messages(klog)

    def dump_info(self, klog):
        rows = [["Mask", as_hex(klog.mask)],
                ["Verbose", str(klog.verbose)],
                ["Messages", str(len(klog))]]
        ptable(rows, header=False, fmt='rl')

    def dump_messages(self, klog):
        rows = [["Time", "Source", "System", "Message"]]
        for entry in klog:
            rows.append(["%.6f" % entry.timestamp.as_float(),
                         "%s:%d" % (entry.source, entry.line),
                         entry.origin, entry.format_msg()])
        ptable(rows, header=True, fmt='rrrl')
