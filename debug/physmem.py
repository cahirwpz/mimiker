import gdb
import tailq
import ptable
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
        rows = [['Segment', 'Start', 'End', 'Pages no']]
        for idx, seg in enumerate(segments):
            start = as_uint32(seg['start'])
            end = as_uint32(seg['end'])
            row = ["%d" % idx, "0x%08x" % start, "0x%08x" % end, "%d" % seg['npages']]
            rows.append(row)
        ptable.ptable(rows, header=True)


################################################################################

class KernelFreePages():

    def invoke(self):
        self.dump_free_pages()

    def get_all_segments(self):
        segq = gdb.parse_and_eval('seglist')
        return tailq.collect_values(segq, 'segq')

    def dump_segment_freeq(self, idx, freeq, size):
        pages = tailq.collect_values(freeq, 'freeq')
        rows = []
        for page in pages:
            paddr = as_uint32(page['paddr'])
            vaddr = as_uint32(page['vaddr'])
            rows.append(["%d" % idx, "%d" % size, "0x%08x" % paddr, "0x%08x" % vaddr])
        return rows

    def dump_segment_free_pages(self, idx, segment):
        rows = []
        for q in range(0, 16):
            rows += self.dump_segment_freeq(idx, segment['freeq'][q], 4 << q)
        return rows


    def dump_free_pages(self):
        segments = self.get_all_segments()
        rows = [['Segment', 'Page size', 'Physical', 'Virtual']]
        for idx, seg in enumerate(segments):
            rows += self.dump_segment_free_pages(idx, seg)
        ptable.ptable(rows, header=True)

################################################################################

class TLB:

    def invoke(self):
        self.dump_tlb()

    def get_tlb_size(self):
        return gdb.parse_and_eval('tlb_size()')

    def get_tlb_entry(self, idx):
        gdb.parse_and_eval('_gdb_tlb_read_index(' + str(idx) + ')')
        return gdb.parse_and_eval('_gdb_tlb_entry')

    #def get_tlb_lo0(self, idx):
    #    return gdb.parse_and_eval('tlb_read_entry_lo0(' + str(idx) + ')')

    #def get_tlb_lo1(self, idx):
    #    return gdb.parse_and_eval('tlb_read_entry_lo1(' + str(idx) + ')')

    def dump_tlb_index(self, idx, hi, lo0, lo1):
        row = []
        if lo0 & 2 or lo1 & 2:
            row += ["%02d" % idx, "0x%02x" % (hi & 0xff)]
            #print "| %02d    | 0x%02x | " % (idx, hi & 0xff),
            if (lo0 & 2):
                _hi = as_uint32(hi & 0xffffe000)
                _lo = as_uint32((lo0 & 0x03ffffc0) << 6)
                row.append("0x%08x => 0x%08x %c%c" % (_hi,
                                                     _lo,
                                                     'D' if (lo0 & 4) else '-',
                                                     'G' if (lo0 & 1) else '-'))
            else:
                row.append("")
            if (lo1 & 2):
                _hi = as_uint32((hi & 0xffffe000) + 4096)
                _lo = as_uint32((lo1 & 0x03ffffc0) << 6)
                row.append("0x%08x => 0x%08x %c%c" % (_hi,
                                                      _lo,
                                                      'D' if (lo1 & 4) else '-',
                                                      'G' if (lo1 & 1) else '-'))
            else:
                row.append("")
        return row


    def dump_tlb(self):
        tlb_size = self.get_tlb_size();
        rows = [["Index", "ASID", "PFN0", "PFN1"]]
        for idx in range(0, tlb_size):
            entry = self.get_tlb_entry(idx)
            #hi = self.get_tlb_hi(idx)
            #lo0 = self.get_tlb_lo0(idx)
            #lo1 = self.get_tlb_lo1(idx)

            row = self.dump_tlb_index(idx,entry['hi'], entry['lo0'], entry['lo1'])
            if (row != []):
                rows.append(row)
        ptable.ptable(rows, fmt="rrll", header=True)