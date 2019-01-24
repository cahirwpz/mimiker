import gdb

from .tailq import TailQueue
from .ptable import ptable, as_hex
from .utils import UserCommand


class KernelSegments(UserCommand):
    """List physical memory segments"""

    def __init__(self):
        super().__init__('segments')

    def get_all_segments(self):
        return TailQueue(gdb.parse_and_eval('seglist'), 'segq')

    def __call__(self, args):
        segments = self.get_all_segments()
        rows = [['segment', 'start', 'end', 'pages']]
        for idx, seg in enumerate(segments):
            rows.append([str(idx), as_hex(seg['start']), as_hex(seg['end']),
                         str(int(seg['npages']))])
        ptable(rows, header=True)


class KernelFreePages(UserCommand):
    """List free pages within physical memory segments"""

    def __init__(self):
        super().__init__('free_pages')

    def get_all_segments(self):
        return TailQueue(gdb.parse_and_eval('seglist'), 'segq')

    def dump_segment_freeq(self, idx, freeq, size):
        pages = TailQueue(freeq, 'freeq')
        return [[str(idx), str(size), as_hex(page['paddr'])]
                for page in pages]

    def dump_segment_free_pages(self, idx, segment):
        helper = []
        for q in range(16):
            helper.extend(self.dump_segment_freeq(
                idx, segment['freeq'][q], 4 << q))
        return helper

    def __call__(self, args):
        segments = self.get_all_segments()
        rows = [['segment', 'page size', 'physical']]
        for idx, seg in enumerate(segments):
            rows.extend(self.dump_segment_free_pages(idx, seg))
        ptable(rows, header=True)
