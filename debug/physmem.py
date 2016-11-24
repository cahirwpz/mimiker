import gdb
import tailq
import struct
import ctypes

################################################################################

def as_uint32(num):
    return ctypes.c_ulong(num).value & 0xffffffff

################################################################################

class KernelSegments():

    def invoke(self):
        self.dump_segments()

    def get_all_segments(self):
        segq = gdb.parse_and_eval('seglist')
        return tailq.collect_values(segq, 'segq')

    def dump_segments(self):
        segments = self.get_all_segments()
        print("------------------------------------------------")
        print("| Segment | Start      | End        | Pages no |")
        print("------------------------------------------------")
        for idx, seg in enumerate(segments):
            #print 'start: ', seg['start'], 'end: ', seg['end']
            start = as_uint32(seg['start'])
            end = as_uint32(seg['end'])
            print("| %7d | 0x%08x | 0x%08x | %8d |" % (idx, start, end, seg['npages']))
        print("------------------------------------------------")

################################################################################

class KernelFreePages():

    def invoke(self):
        self.dump_free_pages()

    def get_all_segments(self):
        segq = gdb.parse_and_eval('seglist')
        return tailq.collect_values(segq, 'segq')

    def dump_segment_freeq(self, idx, freeq, size):
        pages = tailq.collect_values(freeq, 'freeq')
        for page in pages:
            paddr = as_uint32(page['paddr'])
            vaddr = as_uint32(page['vaddr'])
            print("| %7d | %9d | 0x%08x | 0x%08x |" % (idx, size, paddr, vaddr))

    def dump_segment_free_pages(self, idx, segment):
        for q in range(0, 16):
            self.dump_segment_freeq(idx, segment['freeq'][q], 4 << q)


    def dump_free_pages(self):
        segments = self.get_all_segments()
        print("-------------------------------------------------")
        print("| Segment | Page size | Physical   | Virtual    |")
        print("-------------------------------------------------")
        for idx, seg in enumerate(segments):
            self.dump_segment_free_pages(idx, seg)
        print("-------------------------------------------------")

################################################################################

class TLB:

    def invoke(self):
        self.dump_tlb()

    def get_tlb_size(self):
        return gdb.parse_and_eval('tlb_size()')

    def get_tlb_hi(self, idx):
        return gdb.parse_and_eval('tlb_read_entry_hi(' + str(idx) + ')')

    def get_tlb_lo0(self, idx):
        return gdb.parse_and_eval('tlb_read_entry_lo0(' + str(idx) + ')')

    def get_tlb_lo1(self, idx):
        return gdb.parse_and_eval('tlb_read_entry_lo1(' + str(idx) + ')')

    def dump_tlb_index(self, idx, hi, lo0, lo1):
        if lo0 & 2 or lo1 & 2:
            print "| %02d    | 0x%02x | " % (idx, hi & 0xff),
            if (lo0 & 2):
                _hi = as_uint32(hi & 0xffffe000)
                _lo = as_uint32((lo0 & 0x03ffffc0) << 6)
                print "0x%08x => 0x%08x %c%c | " % (_hi,
                                                    _lo,
                                                    'D' if (lo0 & 4) else '-',
                                                    'G' if (lo0 & 1) else '-'),
            else:
                print "                            | ",
            if (lo1 & 2):
                _hi = as_uint32((hi & 0xffffe000) + 4096)
                _lo = as_uint32((lo1 & 0x03ffffc0) << 6)
                print "0x%08x => 0x%08x %c%c |" % (_hi,
                                                   _lo,
                                                   'D' if (lo1 & 4) else '-',
                                                   'G' if (lo1 & 1) else '-')
            else:
                print "                            |"


    def dump_tlb(self):
        tlb_size = self.get_tlb_size();
        print "------------------------------------------------------------------------------"
        print "| Index | ASID | PFN0                         | PFN1                         |"
        print "------------------------------------------------------------------------------"
        for idx in range(0, tlb_size):
            hi = self.get_tlb_hi(idx)
            lo0 = self.get_tlb_lo0(idx)
            lo1 = self.get_tlb_lo1(idx)
            self.dump_tlb_index(idx, hi, lo0, lo1)
        print "------------------------------------------------------------------------------"