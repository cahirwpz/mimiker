import gdb

from .struct import TailQueue
from .cmd import UserCommand
from .cpu import TLBLo
from .utils import TextTable, global_var, cast


PM_NQUEUES = 16


class PhysMap(UserCommand):
    """List active page entries in kernel pmap"""
    def __init__(self):
        super().__init__('pmap')

    def __call__(self, args):
        pdp = global_var('kernel_pmap')['pde']
        table = TextTable(types='ttttt', align='rrrrr')
        table.header(['vpn', 'pte0', 'pte1', 'pte2', 'pte3'])
        for i in range(1024):
            pde = TLBLo(pdp[i])
            if not pde.valid:
                continue
            ptp = pde.ppn.cast(gdb.lookup_type('pte_t').pointer())
            pte = [TLBLo(ptp[j]) for j in range(1024)]
            for j in range(0, 1024, 4):
                if not any(pte.valid for pte in pte[j:j+4]):
                    continue
                pte4 = [str(pte) if pte.valid else '-' for pte in pte[j:j+4]]
                table.add_row(['{:8x}'.format((i << 22) + (j << 12)),
                               pte4[0], pte4[1], pte4[2], pte4[3]])
        print(table)


class VmPhysSeg(UserCommand):
    """List physical memory segments managed by vm subsystem"""

    def __init__(self):
        super().__init__('vm_physseg')

    def __call__(self, args):
        table = TextTable(types='ittit', align='rrrrr')
        table.header(['segment', 'start', 'end', 'pages', 'used'])
        segments = TailQueue(global_var('seglist'), 'seglink')
        for idx, seg in enumerate(segments):
            table.add_row([idx, seg['start'], seg['end'], int(seg['npages']),
                           bool(seg['used'])])
        print(table)


class VmFreePages(UserCommand):
    """List free pages known to vm subsystem"""

    def __init__(self):
        super().__init__('vm_freepages')

    def __call__(self, args):
        table = TextTable(align='rrl', types='iit')
        free_pages = 0
        for q in range(PM_NQUEUES):
            count = int(global_var('pagecount')[q])
            pages = []
            for page in TailQueue(global_var('freelist')[q], 'freeq'):
                pages.append('{:8x}'.format(int(page['paddr'])))
                free_pages += int(page['size'])
            table.add_row([count, 2**q, ' '.join(pages)])
        table.header(['#pages', 'size', 'addresses'])
        print(table)
        print('Free pages count: {}'.format(free_pages))
        segments = TailQueue(global_var('seglist'), 'seglink')
        pages = int(sum(seg['npages'] for seg in segments if not seg['used']))
        print('Used pages count: {}'.format(pages - free_pages))


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
