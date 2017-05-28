import gdb
import tailq
from ptable import ptable
import ctypes

PAGESIZE = 0x1000


def is_global(lo):
    return bool(lo & 1)


def is_valid(lo):
    return bool(lo & 2)


def is_dirty(lo):
    return bool(lo & 4)


def vpn_of(hi):
    return hi & 0xffffe000


def ppn_of(lo):
    return (lo & 0x03ffffc0) << 6


def asid_of(hi):
    return hi & 0x000000ff


def as_uint32(num):
    return ctypes.c_ulong(num).value & 0xffffffff


def as_hex(num):
    return "$%08x" % as_uint32(num)


class KernelSegments():

    def invoke(self):
        self.dump_segments()

    def get_all_segments(self):
        segq = gdb.parse_and_eval('seglist')
        return tailq.collect_values(segq, 'segq')

    def dump_segments(self):
        segments = self.get_all_segments()
        rows = [['Segment', 'Start', 'End', 'Pages no']]
        for idx, seg in enumerate(segments):
            rows.append([str(idx), as_hex(seg['start']), as_hex(seg['end']),
                         str(seg['npages'])])
        ptable(rows, header=True)


class KernelFreePages():

    def invoke(self):
        self.dump_free_pages()

    def get_all_segments(self):
        segq = gdb.parse_and_eval('seglist')
        return tailq.collect_values(segq, 'segq')

    def dump_segment_freeq(self, idx, freeq, size):
        pages = tailq.collect_values(freeq, 'freeq')
        return [[str(idx), str(size), as_hex(page['paddr']),
                 as_hex(page['vaddr'])] for page in pages]

    def dump_segment_free_pages(self, idx, segment):
        helper = []
        for q in range(16):
            helper.extend(self.dump_segment_freeq(
                idx, segment['freeq'][q], 4 << q))
        return helper

    def dump_free_pages(self):
        segments = self.get_all_segments()
        rows = [['Segment', 'Page size', 'Physical', 'Virtual']]
        for idx, seg in enumerate(segments):
            rows.extend(self.dump_segment_free_pages(idx, seg))
        ptable(rows, header=True)


class TLB:

    def invoke(self):
        self.dump_tlb()

    def get_tlb_size(self):
        return gdb.parse_and_eval('tlb_size()')

    def get_tlb_entry(self, idx):
        gdb.parse_and_eval('_gdb_tlb_read_index(%d)' % idx)
        return gdb.parse_and_eval('_gdb_tlb_entry')

    def dump_entrylo(self, vpn, lo):
        if not is_valid(lo):
            return "-"
        vpn = as_hex(vpn)
        ppn = as_hex(ppn_of(lo))
        dirty = '-D'[is_dirty(lo)]
        globl = '-G'[is_global(lo)]
        return "%s => %s %c%c" % (vpn, ppn, dirty, globl)

    def dump_tlb_index(self, idx, hi, lo0, lo1):
        if not is_valid(lo0) and not is_valid(lo1):
            return []
        return ["%02d" % idx, "$%02x" % asid_of(hi),
                self.dump_entrylo(vpn_of(hi), lo0),
                self.dump_entrylo(vpn_of(hi), lo1)]

    def dump_tlb(self):
        tlb_size = self.get_tlb_size()
        rows = [["Index", "ASID", "PFN0", "PFN1"]]
        for idx in range(tlb_size):
            entry = self.get_tlb_entry(idx)
            hi, lo0, lo1 = entry['hi'], entry['lo0'], entry['lo1']
            row = self.dump_tlb_index(idx, hi, lo0, lo1)
            if not row:
                continue
            rows.append(row)
        ptable(rows, fmt="rrll", header=True)


class KernelLog():
    def invoke(self):
        messages, klog = self.load_klog()
        self.dump_general_info(len(messages), klog)
        self.dump_kernel_logs(messages)

    def format_message(self, data):
        message = data['kl_format'].string()
        arguments = data['kl_params']
        # If there is % escaped it is not parameter.
        number_of_parameters = message.count('%') - 2 * message.count('%%')
        params = ', '.join(str(arguments[i])
                           for i in range(number_of_parameters))
        try:
            message = message.replace('"', '\\"')
            # TODO: why do we need %zu???
            message = message.replace('%zu', '%u')
            # Using gdb printf so we don't need to dereference addresses.
            formated = gdb.execute(
                'printf "' + message + ' ", ' + params, to_string=True)
        except Exception as e:
            print('''Error in formating message "{}" with parameters "{}"\n
            Message skipped!!'''.format(message, params))
            formated = message + params
        time = str(data['kl_timestamp']['tv_sec'] * 1e6
                        + data['kl_timestamp']['tv_usec'])
        return [time, str(data['kl_line']), str(data['kl_file'].string()),
                            str(data['kl_origin']), str(formated)]

    def load_klog(self):
        klog = gdb.parse_and_eval('klog')
        first = int(klog['first'])
        last = int(klog['last'])
        klog_array = klog['array']
        array_size = int(klog_array.type.range()[1]) + 1
        number_of_logs = (last - first
                            if last >= first else last + array_size - first)
        messages = []
        while first != last:
            data = klog_array[first]
            messages.append(self.format_message(data))
            first = (first + 1) % array_size
        return messages, klog

    def dump_general_info(self, number_of_logs, klog):
        rows_general = [["Mask", "Verbose", "Log number"]]
        mask = as_hex(klog['mask'])
        verbose = bool(klog['verbose'])
        rows_general.append([mask, str(verbose), str(number_of_logs)])
        ptable(rows_general, header=True, fmt='l')

    def dump_kernel_logs(self, messages):
        rows_data = [["Time", "Line", "File", "Origin", "Message"]]
        for message in messages:
            rows_data.append(message)
        ptable(rows_data, header=True, fmt='ccrcl')
