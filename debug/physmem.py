import gdb

from .tailq import TailQueue
from .ptable import ptable, as_hex


class KernelSegments():

    def invoke(self):
        self.dump_segments()

    def get_all_segments(self):
        return TailQueue(gdb.parse_and_eval('seglist'), 'segq')

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
        return TailQueue(gdb.parse_and_eval('seglist'), 'segq')

    def dump_segment_freeq(self, idx, freeq, size):
        pages = TailQueue(freeq, 'freeq')
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
