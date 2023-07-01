from .struct import TailQueue
from .cmd import UserCommand, CommandDispatcher
from .cpu import TLBLo
from .utils import TextTable, global_var, cast_ptr, get_arch
from .proc import Process


PM_NQUEUES = 16


class VmInfo(CommandDispatcher):
    """Examine virtual memory data structures."""

    def __init__(self):
        super().__init__('vm', [KernelPmap(),
                                UserPmap(),
                                VmMapDump(),
                                CurrentSegmentInfo(),
                                ProcSegmentInfo(),
                                VmPhysSeg(),
                                VmFreePages(),
                                ])


def _print_mips_pmap(pmap):
    pdp = cast_ptr(pmap['pde'], 'pde_t')
    table = TextTable(types='ttttt', align='rrrrr')
    table.header(['vpn', 'pte0', 'pte1', 'pte2', 'pte3'])
    for i in range(1024):
        pde = TLBLo(pdp[i])
        if not pde.valid:
            continue
        ptp = cast_ptr(pde.ppn, 'pte_t')
        pte = [TLBLo(ptp[j]) for j in range(1024)]
        for j in range(0, 1024, 4):
            if not any(pte.valid for pte in pte[j:j+4]):
                continue
            pte4 = [str(pte) if pte.valid else '-' for pte in pte[j:j+4]]
            table.add_row(['{:8x}'.format((i << 22) + (j << 12)),
                           pte4[0], pte4[1], pte4[2], pte4[3]])
    print(table)


class KernelPmap(UserCommand):
    """List active page entries in kernel pmap"""

    def __init__(self):
        super().__init__('pmap_kernel')

    def __call__(self, args):
        pmap = global_var('kernel_pmap')
        arch = get_arch()
        if arch == 'mips':
            _print_mips_pmap(pmap)
        else:
            print(f"Can't print {arch} pmap")


class UserPmap(UserCommand):
    """List active page entries in user pmap

    vm pmap_user [pid]

    Arguments:
        pid    pid of process to show pmap for

    If argument is not specifed the pmap of current process is dumped.
    """

    def __init__(self):
        super().__init__('pmap_user')

    def __call__(self, args):
        args = args.split()
        if len(args) == 0:
            proc = Process.from_current()
        else:
            pid = int(args[0])
            proc = Process.find_by_pid(pid)
            if proc is None:
                print(f'Process {pid} not found')
                return
        pmap = proc.p_uspace.pmap
        arch = get_arch()
        if arch == 'mips':
            _print_mips_pmap(pmap)
        else:
            print(f"Can't print {arch} pmap")


class VmMapDump(UserCommand):
    """List segments describing virtual address space

    vm map [pid]

    Arguments:
        pid    pid of process to list vm_map for

    If argument is not specifed the vm_map of current process is listed.
    """

    def __init__(self):
        super().__init__('map')

    def __call__(self, args):
        args = args.split()
        if len(args) == 0:
            proc = Process.from_current()
        else:
            pid = int(args[0])
            proc = Process.find_by_pid(pid)
            if proc is None:
                print(f'Process {pid} not found')
                return

        entries = proc.p_uspace.get_entries()

        table = TextTable(types='ittttt', align='rrrrrr')
        table.header(['segment', 'start', 'end', 'prot', 'flags', 'amap'])
        for idx, seg in enumerate(entries):
            table.add_row([idx, hex(seg.start), hex(seg.end), seg.prot,
                           seg.flags, seg.aref])
        print(table)


def _print_segment(seg, pid, id):
    print('Segment {} in proc {}'.format(id, pid))
    print('Range: {:#08x}-{:#08x} ({:d} pages)'.format(seg.start,
                                                       seg.end,
                                                       seg.pages))
    print('Prot:  {}'.format(seg.prot))
    print('Flags: {}'.format(seg.flags))
    amap = seg.amap
    if amap:
        print('Amap:        {}'.format(seg.amap_ptr))
        print('Amap offset: {}'.format(seg.amap_offset))
        print('Amap slots:  {}'.format(amap.slots))
        print('Amap refs:   {}'.format(amap.ref_cnt))

        amap.print_pages(seg.amap_offset, seg.pages)
    else:
        print('Amap: NULL')


def _segment_info(proc, segment):
    entries = proc.p_uspace.get_entries()
    if segment < 4096:
        # Lookup by id
        if segment > len(entries):
            print(f'Segment {segment} does not exist!')
            return
        _print_segment(entries[segment], proc.p_pid, segment)
    else:
        # Lookup by address
        addr = segment
        for idx, e in enumerate(entries):
            if e.start <= addr and addr < e.end:
                _print_segment(e, proc.p_pid, idx)
                return
        print(f'Segment with address {addr} not found')


class CurrentSegmentInfo(UserCommand):
    """Show info about i-th segment in curent proc vm_map"""

    def __init__(self):
        super().__init__('segment')

    def __call__(self, args):
        args = args.split()
        if len(args) < 1:
            print('require argument (segment)')
            return
        proc = Process.from_current()
        segment = int(args[0], 0)
        _segment_info(proc, segment)


class ProcSegmentInfo(UserCommand):
    """Show info about i-th segment in proc vm_map"""

    def __init__(self):
        super().__init__('segment_proc')

    def __call__(self, args):
        args = args.split()
        if len(args) < 2:
            print('require 2 arguments (pid and segment)')
            return

        pid = int(args[0])
        proc = Process.find_by_pid(pid)
        if proc is None:
            print(f'Process {pid} not found')
            return
        segment = int(args[1], 0)
        _segment_info(proc, segment)


class VmPhysSeg(UserCommand):
    """List physical memory segments managed by vm subsystem"""

    def __init__(self):
        super().__init__('physseg')

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
        super().__init__('freepages')

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
