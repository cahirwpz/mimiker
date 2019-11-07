import gdb

mimiker_current_p_elfpath = ''

def get_p_elfpath():
        pcpu_data = gdb.newest_frame().read_var('_pcpu_data')
        curthread_ptr = pcpu_data.dereference()['curthread']
        proc_ptr = curthread_ptr.dereference()['td_proc']
        p_elfpath = proc_ptr.dereference()['p_elfpath']
        return p_elfpath


class UserReturnBP(gdb.Breakpoint):
    def __init__(self):
        #super().__init__(label='user_return', internal=True)
        super().__init__('user_return', internal=True)
        self.p_elfpath = ''
        self.fired = 0

    def stop(self):
        self.fired += 1
        p_elfpath = get_p_elfpath()
        self.p_elfpath = p_elfpath

        global mimiker_current_p_elfpath
        mimiker_current_p_elfpath = str(p_elfpath).split(',', 1)[0][1:-1]

        return False

def get_stop_handler(elves):

    def stop_handler(event):
        user_elf_base_addr = 0x00400000
        kernel_base = 0x80000000    
    
        pc = gdb.parse_and_eval('$pc')
        pc = int(pc) &  0xffffffff
    
        if pc > kernel_base:
            return

        try:
            gdb.execute(
                f'remove-symbol-file -a {hex(user_elf_base_addr)}')
        except:
            print(f"no symbol file loaded at {hex(user_elf_base_addr)}")
        
        mimiker_path = mimiker_current_p_elfpath
        host_path = elves[mimiker_path]
    
        gdb.execute(
            f'add-symbol-file  {host_path}')
    return stop_handler
