import gdb
import utils
from ptable import ptable
from tailq import TailQueue


class PoolSet:
    @staticmethod
    def __is_non_static(decl):
        return (len(decl) == 2) and (decl[0] == 'kmem_pool_t')

    @staticmethod
    def __is_static(decl):
        return (len(decl) == 3) and (decl[0] == 'static') \
               and (decl[1] == 'kmem_pool_t')

    @staticmethod
    def get_global_pools():
        decls_string = gdb.execute('info variables', False, True)
        decls = decls_string.split('\n')
        result = []
        for decl in decls:
            decl = decl.split()
            if PoolSet.__is_non_static(decl):
                varname = decl[1][1:-1]
                result.append(varname)
            if PoolSet.__is_static(decl):
                varname = decl[2][1:-1]
                result.append(varname)

        return result

    @staticmethod
    def __is_local_pool(name):
        type_decl = gdb.execute('whatis %s' % name, False, True)
        return (type_decl == "type = kmem_pool_t *\n")

    @staticmethod
    def get_local_pools():
        decls_string = gdb.execute('info locals', False, True)
        decls = decls_string.split('\n')
        result = []

        if (decls[0] == ''):
            return

        for decl in decls[:-1]:
            name = decl.split()[0]
            if PoolSet.__is_local_pool(name):
                result.append(name)

        return result


class Pool():
    __metaclass__ = utils.GdbStructMeta
    __ctype__ = 'kmem_pool_t'
    __cast__ = {'mp_desc': str, 'mp_pages_used': str, 'mp_pages_max': str,
                'mp_arena': lambda x: map(Arena,
                                          TailQueue(field='ma_list', tq=x))}

    def __init__(self, obj, name):
        super(Pool, self).__init__(obj)
        self.name = name
        self.address = str(obj.address)

    @staticmethod
    def get_table_header():
        return [["Pool name", "Pool descr.",
                 "Pages used", "Pages max."]]

    def get_row(self):
        return [self.name, self.mp_desc, self.mp_pages_used, self.mp_pages_max]


class Arena():
    __metaclass__ = utils.GdbStructMeta
    __ctype__ = 'mem_arena_t'
    __cast__ = {'ma_size': str, 'ma_flags': str, 'ma_freeblks':
                lambda x: map(Block, TailQueue(field='mb_list', tq=x))}

    def __init__(self, obj):
        super(Arena, self).__init__(obj)
        self.address = str(obj.address)

    @staticmethod
    def get_header():
        return [["arena addr.", "ma_size", "ma_flags", "ma_freeblks first"]]

    def get_row(self):
        return [self.address, self.ma_size, self.ma_flags,
                str(iter(self.ma_freeblks).next().address)]


class Block():
    __metaclass__ = utils.GdbStructMeta
    __ctype__ = 'mem_block_t'
    __cast__ = {'mb_size': str, 'mb_data': str}

    def __init__(self, obj):
        super(Block, self).__init__(obj)
        self.address = str(obj.address)

    @staticmethod
    def get_header():
        return [["block addr", "mb_size", "mb_data addr."]]

    def get_row(self):
        return [self.address, self.mb_size, self.mb_data]


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
        self.pool_names = PoolSet.get_global_pools()

    def add_row_to_pool_table(self, table, pool):
        pool_ptr = gdb.parse_and_eval('%s' % pool)
        name = str('%s' % pool)
        p = Pool(pool_ptr, name)
        table.append(p.get_row())

    def dump_all_pools(self):
        tbl = Pool.get_table_header()

        for pool in self.pool_names:
            self.add_row_to_pool_table(tbl, pool)

        ptable(tbl, fmt="cccc", header=True)

    def dump_pool(self, pool):
        if self.is_valid_pool(pool):
            tbl = Pool.get_table_header()
            self.add_row_to_pool_table(tbl, pool)
            ptable(tbl, fmt="cccc", header=True)
        else:
            print("Invalid pool name\n")

    def is_valid_pool(self, pool):
        return pool in self.pool_names

    def dump_arenas(self, pool):
        tbl = Arena.get_header()
        pool_ptr = gdb.parse_and_eval('%s' % pool)

        p = Pool(pool_ptr, pool)
        for arena in p.mp_arena:
            tbl.append(arena.get_row())

        print 'Arenas of %s pool' % pool
        ptable(tbl, fmt="cccc", header=True)

    def dump_blocks(self, pool):

        pool_ptr = gdb.parse_and_eval('%s' % pool)
        p = Pool(pool_ptr, pool)

        for arena in p.mp_arena:
            arena_addr = arena.address
            print 'Arena addr: %s' % str(arena_addr)
            tbl = Block.get_header()

            block_queue = arena.ma_freeblks
            for block in block_queue:
                tbl.append(block.get_row())
            ptable(tbl, fmt="ccc", header=True)

    def invoke(self, args, from_tty):
        self.pool_names = PoolSet.get_global_pools()

        if len(args) < 1:
            self.dump_all_pools()
        else:
            self.pool_names += PoolSet.get_local_pools()
            self.dump_pool(args)
            self.dump_arenas(args)
            self.dump_blocks(args)

    def options(self):
        return PoolSet.get_global_pools() + \
            PoolSet.get_local_pools()
