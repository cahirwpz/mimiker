import gdb

from .cmd import UserCommand
from .struct import List, TailQueue, LinkerSet
from .utils import global_var, TextTable


class Vmem(UserCommand):
    """List vmem boundary tags and usage summary."""

    def __init__(self):
        super().__init__('vmem')

    def __call__(self, args):
        vmem_list = List(global_var('vmem_list'), 'vm_link')
        for vmem in vmem_list:
            print('Vmem name: "{}"\n'.format(vmem['vm_name'].string()))
            table = TextTable(types='ttt', align='rrl')
            table.header(['start', 'size', 'type'])
            used = 0
            total = 0
            for bt in TailQueue(vmem['vm_seglist'], 'bt_seglink'):
                bt_type = str(bt['bt_type'])[8:]
                bt_size = int(bt['bt_size'])
                if bt_type == 'SPAN':
                    continue
                if bt_type == 'BUSY':
                    used += bt_size
                total += bt_size
                table.add_row([bt['bt_start'], bt['bt_size'], bt_type])
            print(table)
            print('Used space: 0x{:x}/0x{:x} ({:.2f}%)'.format(
                  used, total, 100.0 * used / total))


class MallocStats(UserCommand):
    """List memory statistics of all malloc pools."""

    def __init__(self):
        super().__init__('malloc_stats')

    def __call__(self, args):
        mps = LinkerSet('kmalloc_pool', 'kmalloc_pool_t *')
        table = TextTable(types='tiiii', align='lrrrr')
        table.header(['description', 'nrequests', 'active', 'memory in use',
                      'peak usage'])
        for mp in sorted(mps, key=lambda x: x['desc'].string()):
            table.add_row([mp['desc'].string(), int(mp['nrequests']),
                           int(mp['active']), int(mp['used']),
                           int(mp['maxused'])])
        print(table)


class PoolStats(UserCommand):
    """List memory statistics of all object pools."""

    def __init__(self):
        super().__init__('pool_stats')

    def __call__(self, args):
        pool_list = TailQueue(global_var('pool_list'), 'pp_link')
        table = TextTable(types='tiiii', align='lrrrr')
        table.header(['description', 'bytes', 'used items', 'max used items',
                      'total items'])
        for pool in sorted(pool_list, key=lambda x: x['pp_desc'].string()):
            table.add_row([pool['pp_desc'].string(), int(pool['pp_npages']),
                           int(pool['pp_nused']), int(pool['pp_nmaxused']),
                           int(pool['pp_ntotal'])])
        print(table)
