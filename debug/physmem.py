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
            rows.append([idx, as_hex(seg['start']), as_hex(seg['end']),
                         seg['npages']])
        ptable(rows, header=True)


class KernelFreePages():

    def invoke(self):
        self.dump_free_pages()

    def get_all_segments(self):
        segq = gdb.parse_and_eval('seglist')
        return tailq.collect_values(segq, 'segq')

    def dump_segment_freeq(self, idx, freeq, size):
        pages = tailq.collect_values(freeq, 'freeq')
        return [[idx, size, as_hex(page['paddr']), as_hex(page['vaddr'])]
                for page in pages]

    def dump_segment_free_pages(self, idx, segment):
        return [self.dump_segment_freeq(idx, segment['freeq'][q], 4 << q)
                for q in range(16)]

    def dump_free_pages(self):
        segments = self.get_all_segments()
        rows = [['Segment', 'Page size', 'Physical', 'Virtual']]
        for idx, seg in enumerate(segments):
            rows.append(self.dump_segment_free_pages(idx, seg))
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
