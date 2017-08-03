from __future__ import print_function
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


class OutputCreator():
    @staticmethod
    def get_pool_header():
        return [["Pool name", "Pool descr.",
                 "Pages used", "Pages max."]]

    @staticmethod
    def get_pool_table_row(p):
        return [p.name, p.mp_desc, str(p.mp_pages_used),
                str(p.mp_pages_max)]

    @staticmethod
    def get_arena_header():
        return [["arena addr.", "ma_size", "ma_flags", "ma_freeblks first"]]

    @staticmethod
    def get_arena_row(a):
        return [str(a.address), str(a.ma_size), str(a.ma_flags),
                str(iter(a.ma_freeblks).next().address)]

    @staticmethod
    def get_block_header():
        return [["block addr", "mb_size", "mb_data addr."]]

    @staticmethod
    def get_block_row(b):
        return [str(b.address), str(b.mb_size), str(b.mb_data)]

    
    @staticmethod
    def dump_all_pools(pools):
        tbl = OutputCreator.get_pool_header()

        tbl.extend([OutputCreator.get_pool_table_row(pool) for pool in pools ])

        ptable(tbl, fmt="cccc", header=True)

    @staticmethod    
    def dump_pool(pool):
        tbl = OutputCreator.get_pool_header()
        tbl.append(OutputCreator.get_pool_table_row(pool))
        ptable(tbl, fmt="cccc", header=True)

 
    @staticmethod
    def dump_arenas(pool):
        tbl = OutputCreator.get_arena_header()
        tbl.extend([OutputCreator.get_arena_row(a) for a in pool.mp_arena])

        print('Arenas of %s pool' % pool.name)
        ptable(tbl, fmt="cccc", header=True)

        
    @staticmethod
    def dump_blocks(pool):
        for arena in pool.mp_arena:
            arena_addr = arena.address
            print('Arena addr: %s' % str(arena_addr))
            tbl = OutputCreator.get_block_header()

            block_queue = arena.ma_freeblks
            tbl.extend([OutputCreator.get_block_row(b) for b in block_queue])
            ptable(tbl, fmt="ccc", header=True)
       

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

        

    @staticmethod
    def pool_from_name(name):
        pool_ptr = gdb.parse_and_eval('%s' % name)
        return Pool(pool_ptr, name)
        
    
    def is_valid_pool(self, name):
        return name in self.pool_names

 
    def invoke(self, args, from_tty):
        self.pool_names = PoolSet.get_global_pools()

        if len(args) < 1:
            OutputCreator.dump_all_pools([Kmem.pool_from_name(x) for x
                                          in self.pool_names])

        else:
            self.pool_names += PoolSet.get_local_pools()
            if self.is_valid_pool(args):
                pool = Kmem.pool_from_name(args)
                OutputCreator.dump_pool(pool)
                OutputCreator.dump_arenas(pool)
                OutputCreator.dump_blocks(pool)      
            else:
                print("Invalid pool name\n")


    def options(self):
        return PoolSet.get_global_pools() + \
            PoolSet.get_local_pools()
