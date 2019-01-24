import gdb

from .tailq import TailQueue
from .utils import UserCommand, TextTable


class KernelSegments(UserCommand):
    """List physical memory segments"""

    def __init__(self):
        super().__init__('segments')

    def get_all_segments(self):
        return TailQueue(gdb.parse_and_eval('seglist'), 'segq')

    def __call__(self, args):
        segments = self.get_all_segments()
        table = TextTable(types='itti', align='rrrr')
        table.header(['segment', 'start', 'end', 'pages'])
        for idx, seg in enumerate(segments):
            table.add_row([idx, seg['start'], seg['end'], int(seg['npages'])])
        print(table)


class KernelFreePages(UserCommand):
    """List free pages within physical memory segments"""

    def __init__(self):
        super().__init__('free_pages')

    def get_all_segments(self):
        return TailQueue(gdb.parse_and_eval('seglist'), 'segq')

    def dump_segment_freeq(self, idx, freeq, size):
        pages = TailQueue(freeq, 'freeq')
        return [[idx, size, hex(page['paddr'])] for page in pages]

    def dump_segment_free_pages(self, idx, segment):
        helper = []
        for q in range(16):
            helper.extend(self.dump_segment_freeq(
                idx, segment['freeq'][q], 4 << q))
        return helper

    def __call__(self, args):
        segments = self.get_all_segments()
        table = TextTable(align='rrr')
        for idx, seg in enumerate(segments):
            table.add_rows(self.dump_segment_free_pages(idx, seg))
        table.header(['segment', '#pages', 'phys addr'])
        print(table)
