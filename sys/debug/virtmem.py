from .struct import TailQueue
from .cmd import UserCommand, CommandDispatcher
from .cpu import PageTable
from .utils import TextTable, global_var
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


class KernelPmap(UserCommand):
    """List active page entries in kernel pmap"""

    def __init__(self):
        super().__init__('pmap_kernel')

    def __call__(self, args):
        pmap = PageTable(global_var('kernel_pmap'))
        pmap.print()


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
        pmap = PageTable(proc.p_uspace.pmap)
        pmap.print()


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
    print(f'Segment {id} in proc {pid}')
    print(f'Range: {seg.start:#08x}-{seg.end:#08x} ({seg.pages:d} pages)')
    print(f'Prot:  {seg.prot}')
    print(f'Flags: {seg.flags}')
    amap = seg.amap
    if amap:
        print(f'Amap:        {seg.amap_ptr}')
        print(f'Amap offset: {seg.amap_offset}')
        print(f'Amap slots:  {amap.slots}')
        print(f'Amap refs:   {amap.refcnt}')

        amap.print_pages(seg.amap_offset, seg.pages)
    else:
        print('Amap: NULL')


def _segment_info(proc, segment):
    MAX_SEGMENT_ID = 4096
    entries = proc.p_uspace.get_entries()
    if segment < MAX_SEGMENT_ID:
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
                pages.append(f'{int(page["paddr"]):8x}')
                free_pages += int(page['size'])
            table.add_row([count, 2**q, ' '.join(pages)])
        table.header(['#pages', 'size', 'addresses'])
        print(table)
        print(f'Free pages count: {free_pages}')
        segments = TailQueue(global_var('seglist'), 'seglink')
        pages = int(sum(seg['npages'] for seg in segments if not seg['used']))
        print(f'Used pages count: {pages - free_pages}')
