from __future__ import print_function
import gdb
import utils
import vars
from ptable import ptable
from tailq import TailQueue


class Pool():
    __metaclass__ = utils.GdbStructMeta
    __ctype__ = 'kmem_pool_t'
    __cast__ = {'mp_desc': str, 'mp_pages_used': int,
                'mp_pages_max': int, 'mp_magic': int,
                'mp_arena': lambda x: map(Arena,
                                          TailQueue(field='ma_list', tq=x))}

    def __init__(self, obj, name):
        super(Pool, self).__init__(obj)
        self.name = name

    def is_valid(self):
        return self.mp_magic == 0xC0DECAFE


class Arena():
    __metaclass__ = utils.GdbStructMeta
    __ctype__ = 'mem_arena_t'
    __cast__ = {'ma_size': int, 'ma_flags': int, 'ma_magic': int,
                'ma_freeblks':
                lambda x: map(Block, TailQueue(field='mb_list', tq=x))}

    def __init__(self, obj):
        super(Arena, self).__init__(obj)

    def is_valid(self):
        return self.ma_magic == 0xC0DECAFE


class Block():
    __metaclass__ = utils.GdbStructMeta
    __ctype__ = 'mem_block_t'
    __cast__ = {'mb_size': int, 'mb_magic': int, 'mb_data': str}

    def __init__(self, obj):
        super(Block, self).__init__(obj)

    def is_valid(self):
        return self.mb_magic == 0xC0DECAFE


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
        return vars.has_type(var, 'kmem_pool_t *')

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

    @staticmethod
    def __kmem_dump(pool):
        if not pool.is_valid():
            print('Pool is corrupted!')
            return
        print(Kmem.__get_pool_descr(pool))

        for arena in pool.mp_arena:
            if not arena.is_valid():
                print('\t Arena is corrupted!')
                continue
            print('\t' + Kmem.__get_arena_descr(arena))
            for block in arena.ma_freeblks:
                if not block.is_valid():
                    print('\t\t Block is corrupted!')
                    continue
                print('\t\t' + Kmem.__get_block_descr(block))

    @staticmethod
    def __get_pool_descr(p):
        return "pool at %s, mp_pages_used = %i, mp_pages_max = %i" \
            % (str(p.address), p.mp_pages_used, p.mp_pages_max)

    @staticmethod
    def __get_arena_descr(a):
        data_start = a.ma_data.address.cast(gdb.lookup_type('size_t'))
        b = int(data_start)
        return "malloc_arena 0x%x - 0x%x " % (data_start, b + a.ma_size)

    @staticmethod
    def __get_block_descr(b):
        return "%s %s %i" % (('F' if (b.mb_size > 0) else 'U'),
                             str(b.address), abs(b.mb_size))

    @staticmethod
    def __dump_pools(pools):
        def _get_pool_table_row(p):
            if p.is_valid():
                return [p.name, p.mp_desc, str(p.mp_pages_used),
                        str(p.mp_pages_max)]
            else:
                return [p.name, 'Pool corrupted!', '---', '---']

        tbl = [["Pool name", "Pool descr.",
                "Pages used", "Pages max."]]
        tbl.extend([_get_pool_table_row(pool) for pool in pools])
        ptable(tbl, fmt="cccc", header=True)

    def invoke(self, args, from_tty):
        if len(args) < 1:
            Kmem.__dump_pools([Kmem.__pool_from_name(var) for var
                               in self.pool_names])

        else:
            if args in self.pool_names + Kmem.__local_pool_names():
                pool = Kmem.__pool_from_name(args)
                Kmem.__kmem_dump(pool)
            else:
                print("Invalid pool name\n")

    def options(self):
        return self.pool_names + \
            Kmem.__local_pool_names()
