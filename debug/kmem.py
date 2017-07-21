import gdb
import utils



class Kmem(gdb.Command, utils.OneArgAutoCompleteMixin):
    """dump info about kernel memory pools 
    
    Syntax is: "kmem arg", where arg is one of pool names defined by 
    MALLOC_DEFINE(), e.g: 
    M_FILE, M_DEV, M_VMOBJ, M_THREAD, M_FD, M_PROC, M_VNODE, M_VFS, M_TEMP, 
    M_VMMAP, M_INITRD, M_PMAP
    """

    pools = {'M_FILE', 'M_DEV', 'M_VMOBJ', 'M_THREAD', 'M_FD', 'M_PROC',
             'M_VNODE', 'M_VFS', 'M_TEMP', 'M_VMMAP', 'M_INITRD', 'M_PMAP'}
    
    def __init__(self):
        super(Kmem, self).__init__('kmem', gdb.COMMAND_USER)
       
            
    def dump_pool(self, pool):
        if self.valid_pool(pool):
            gdb.parse_and_eval('kmem_dump(%s)' % pool)
        else:
            print("Invalid pool name\n");

    def valid_pool(self, pool):
        return pool in self.pools

            
    def invoke(self, args, from_tty):
        self.dump_pool(args)
