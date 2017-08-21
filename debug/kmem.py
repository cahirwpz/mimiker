from __future__ import print_function
import gdb
import utils
import vars
from ptable import ptable
from tailq import TailQueue


class Pool():
    __metaclass__ = utils.GdbStructMeta
    __ctype__ = 'kmem_pool_t'
    __cast__ = {'mp_desc': str, 'mp_pages_used': int, 'mp_pages_max': int,
                'mp_arena': lambda x: map(Arena,
                                          TailQueue(field='ma_list', tq=x))}

    def __init__(self, obj, name):
        super(Pool, self).__init__(obj)
        self.name = name


class Arena():
    __metaclass__ = utils.GdbStructMeta
    __ctype__ = 'mem_arena_t'
    __cast__ = {'ma_size': int, 'ma_flags': int, 'ma_freeblks':
                lambda x: map(Block, TailQueue(field='mb_list', tq=x))}

    def __init__(self, obj):
        super(Arena, self).__init__(obj)


class Block():
    __metaclass__ = utils.GdbStructMeta
    __ctype__ = 'mem_block_t'
    __cast__ = {'mb_size': int, 'mb_data': str}

    def __init__(self, obj):
        super(Block, self).__init__(obj)


class Kmem(gdb.Command, utils.OneArgAutoCompleteMixin):
    """dump info about kernel memory pools

    Syntax is: "kmem arg", where arg is one of pool names defined by
    MALLOC_DEFINE(), e.g:
    M_FILE, M_DEV, M_VMOBJ, M_THREAD, M_FD, M_PROC, M_VNODE, M_VFS, M_TEMP,
    M_VMMAP, M_INITRD, M_PMAP

    kmem without arguments prints a short summary of globally defined
    pools. Locally defined pools are only accessible using "kmem arg"
    command.

    """

    def __init__(self):
        super(Kmem, self).__init__('kmem', gdb.COMMAND_USER)
        self.pool_names = Kmem.__global_pool_names()

    @staticmethod
    def __is_local_pool_name(var):
        return vars.has_type(var, 'kmem_pool_t *')

    @staticmethod
    def __is_global_pool_name(var):
        return vars.has_type(var, 'kmem_pool_t *') or \
            vars.has_type(var, 'static kmem_pool_t *')

    @staticmethod
    def __local_pool_names():
        return [var for var in vars.set_of_locals()
                if Kmem.__is_local_pool_name(var)]

    @staticmethod
    def __global_pool_names():
        return [var for var in vars.set_of_globals()
                if Kmem.__is_global_pool_name(var)]

    @staticmethod
    def __pool_from_name(name):
        pool_ptr = gdb.parse_and_eval(name)
        return Pool(pool_ptr, name)

    def invoke(self, args, from_tty):
        if len(args) < 1:
            _dump_pools([Kmem.__pool_from_name(var) for var
                        in self.pool_names])

        else:
            if args in self.pool_names + Kmem.__local_pool_names():
                pool = Kmem.__pool_from_name(args)
                _dump_pool(pool)
                _dump_arenas(pool)
                _dump_blocks(pool)
            else:
                print("Invalid pool name\n")

    def options(self):
        return self.pool_names + \
            Kmem.__local_pool_names()


def _get_pool_header():
    return [["Pool name", "Pool descr.",
             "Pages used", "Pages max."]]


def _get_pool_table_row(p):
    return [p.name, p.mp_desc, str(p.mp_pages_used),
            str(p.mp_pages_max)]


def _get_arena_header():
    return [["arena addr.", "ma_size", "ma_flags", "ma_freeblks first"]]


def _get_arena_row(a):
    return [str(a.address), str(a.ma_size), str(a.ma_flags),
            str(iter(a.ma_freeblks).next().address)]


def _get_block_header():
    return [["block addr", "mb_size", "mb_data addr."]]


def _get_block_row(b):
    return [str(b.address), str(b.mb_size), str(b.mb_data)]


def _dump_pools(pools):
    tbl = _get_pool_header()
    tbl.extend([_get_pool_table_row(pool) for pool in pools])
    ptable(tbl, fmt="cccc", header=True)


def _dump_pool(pool):
    tbl = _get_pool_header()
    tbl.append(_get_pool_table_row(pool))
    ptable(tbl, fmt="cccc", header=True)


def _dump_arenas(pool):
    tbl = _get_arena_header()
    tbl.extend([_get_arena_row(a) for a in pool.mp_arena])

    print('Arenas of %s pool' % pool.name)
    ptable(tbl, fmt="cccc", header=True)


def _dump_blocks(pool):
    for arena in pool.mp_arena:
        print('Arena addr: %s' % str(arena.address))
        tbl = _get_block_header()

        block_queue = arena.ma_freeblks
        tbl.extend([_get_block_row(b) for b in block_queue])
        ptable(tbl, fmt="ccc", header=True)
