from .struct import TailQueue
from .cmd import UserCommand
from .utils import TextTable, global_var


class KernelSegments(UserCommand):
    """List physical memory segments"""

    def __init__(self):
        super().__init__('segments')

    def __call__(self, args):
        table = TextTable(types='itti', align='rrrr')
        table.header(['segment', 'start', 'end', 'pages'])
        segments = TailQueue(global_var('seglist'), 'segq')
        for idx, seg in enumerate(segments):
            table.add_row([idx, seg['start'], seg['end'], int(seg['npages'])])
        print(table)


class KernelFreePages(UserCommand):
    """List free pages within physical memory segments"""

    def __init__(self):
        super().__init__('free_pages')

    def __call__(self, args):
        table = TextTable(align='rrr')
        segments = TailQueue(global_var('seglist'), 'segq')
        for idx, segment in enumerate(segments):
            for q in range(16):
                size = 4 << q
                for page in TailQueue(segment['freeq'][q], 'freeq'):
                    table.add_row([idx, size, hex(page['paddr'])])
        table.header(['segment', '#pages', 'phys addr'])
        print(table)
