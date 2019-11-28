import gdb

from .struct import TailQueue
from .cmd import UserCommand
from .utils import TextTable, global_var


class VmPhysSeg(UserCommand):
    """List physical memory segments managed by vm subsystem"""

    def __init__(self):
        super().__init__('vm_physseg')

    def __call__(self, args):
        table = TextTable(types='itti', align='rrrr')
        table.header(['segment', 'start', 'end', 'pages'])
        segments = TailQueue(global_var('seglist'), 'seglink')
        for idx, seg in enumerate(segments):
            table.add_row([idx, seg['start'], seg['end'], int(seg['npages'])])
        print(table)


class VmFreePages(UserCommand):
    """List free pages known to vm subsystem"""

    def __init__(self):
        super().__init__('vm_freepages')

    def __call__(self, args):
        table = TextTable(align='rr')
        npages = 0
        for q in range(16):
            for page in TailQueue(global_var('freelist')[q], 'freeq'):
                size = int(page['size'])
                table.add_row([size, hex(page['paddr'])])
                npages += size
        table.header(['#pages', 'phys addr'])
        print(table)
        print('Free pages count: {}'.format(npages))


class VmMapSeg(UserCommand):
    """List segments describing virtual address space"""

    def __init__(self):
        super().__init__('vm_map')

    def __call__(self, args):
        args = args.strip()
        if args not in ['user', 'kernel']:
            print('Choose either "user" or "kernel" vm_map!')
            return
        vm_map = gdb.parse_and_eval('vm_map_%s()' % args)
        if vm_map == 0:
            print('No active %s vm_map!' % args)
            return
        entries = vm_map['entries']
        table = TextTable(types='itttt', align='rrrrr')
        table.header(['segment', 'start', 'end', 'prot', 'object'])
        segments = TailQueue(entries, 'link')
        for idx, seg in enumerate(segments):
            table.add_row([idx, seg['start'], seg['end'], seg['prot'],
                           seg['object']])
        print(table)
