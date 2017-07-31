import gdb
import utils
from ptable import ptable
from tailq import TailQueue


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
        self.pool_names = Kmem.get_global_pools()

    @staticmethod
    def is_non_static(decl):
        return (len(decl) == 2) and (decl[0] == 'kmem_pool_t')

    @staticmethod
    def is_static(decl):
        return (len(decl) == 3) and (decl[0] == 'static') \
               and (decl[1] == 'kmem_pool_t')
        
    @staticmethod
    def get_global_pools():
        decls_string = gdb.execute('info variables', False, True)
        decls = decls_string.split('\n')
        result = []
        for decl in decls:
            decl = decl.split()
            if Kmem.is_non_static(decl):
                varname = decl[1][1:-1]
                result.append(varname)
            if Kmem.is_static(decl):
                varname = decl[2][1:-1]
                result.append(varname)

        return result

    @staticmethod
    def is_local_pool(name):
        type_decl = gdb.execute('whatis %s' % name, False, True)        
        return (type_decl == "type = kmem_pool_t *\n")

    @staticmethod
    def get_local_pools():
        decls_string = gdb.execute('info locals', False, True)
        decls = decls_string.split('\n');
        result = []

        if (decls[0] == ''):
            return
        
        for decl in decls[:-1]:
            name = decl.split()[0]        
            if Kmem.is_local_pool(name):
                result.append(name)

        return result

    
    def get_pool_table_header(self):
        return [["Pool name", "Pool descr.",
                 "Pages used", "Pages max."]]

    def add_row_to_pool_table(self, table, pool):
        pool_ptr = gdb.parse_and_eval('%s' % pool)
        pool_val = pool_ptr.dereference()
        name = str('%s' % pool)
        mp_desc = str(pool_val['mp_desc'])
        mp_pages_used = str(pool_val['mp_pages_used'])
        mp_pages_max = str(pool_val['mp_pages_max'])
        table.append([name, mp_desc, mp_pages_used, mp_pages_max])

    def dump_all_pools(self):
        tbl = self.get_pool_table_header()

        for pool in self.pool_names:
            self.add_row_to_pool_table(tbl, pool)

        ptable(tbl, fmt="cccc", header=True)

    def dump_pool(self, pool):
        if self.is_valid_pool(pool):
            tbl = self.get_pool_table_header()
            self.add_row_to_pool_table(tbl, pool)
            ptable(tbl, fmt="cccc", header=True)
        else:
            print("Invalid pool name\n")

    def is_valid_pool(self, pool):
        return pool in self.pool_names

    def get_arena_header(self):
        return [["arena addr.", "ma_size", "ma_flags", "ma_freeblks first"]]

    def dump_arenas(self, pool):
        tbl = self.get_arena_header()
        pool_ptr = gdb.parse_and_eval('%s' % pool)
        pool_val = pool_ptr.dereference()
        arena_queue = TailQueue(pool_val['mp_arena'], 'ma_list')

        for arena in arena_queue:
            addr = str(arena['ma_list']['tqe_prev'].dereference())
            size = str(arena['ma_size'])
            flags = str(arena['ma_flags'])
            freeblks = str(arena['ma_freeblks']['tqh_first'])
            tbl.append([addr, size, flags, freeblks])

        print 'Arenas of %s pool' % pool
        ptable(tbl, fmt="cccc", header=True)

    def get_block_header(self):
        return [["block addr", "mb_size", "mb_data addr."]]

    def dump_blocks(self, pool):
        pool_ptr = gdb.parse_and_eval('%s' % pool)
        pool_val = pool_ptr.dereference()
        arena_queue = TailQueue(pool_val['mp_arena'], 'ma_list')

        for arena in arena_queue:
            arena_addr = arena.address
            print 'Arena addr: %s' % str(arena_addr)
            tbl = self.get_block_header()

            block_queue = TailQueue(arena['ma_freeblks'], 'mb_list')
            for block in block_queue:
                block_addr = str(block.address)
                mb_size = str(block['mb_size'])
                mb_data = str(block['mb_data'].address)
                tbl.append([block_addr, mb_size, mb_data])

            ptable(tbl, fmt="ccc", header=True)

    def invoke(self, args, from_tty):
        self.pool_names = Kmem.get_global_pools()
        
        if len(args) < 1:
            self.dump_all_pools()
        else:
            self.pool_names += Kmem.get_local_pools()
            self.dump_pool(args)
            self.dump_arenas(args)
            self.dump_blocks(args)


    def options(self):
        
        return Kmem.get_global_pools() + \
            Kmem.get_local_pools()
            
    
