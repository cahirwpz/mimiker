import gdb

from .struct import TailQueue
from .cmd import UserCommand
from .utils import TextTable, global_var


class VMMapSegments(UserCommand):
    """List segments of given vm_map structure"""

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
