import gdb

mimiker_current_p_elfpath = ''

def get_p_elfpath():
        pcpu_data = gdb.newest_frame().read_var('_pcpu_data')
        curthread_ptr = pcpu_data.dereference()['curthread']
        print(f'curthread_ptr= {curthread_ptr}')
        proc_ptr = curthread_ptr.dereference()['td_proc']
        print(f'proc_ptr = {proc_ptr}')
        if proc_ptr == 0:
            return None
        p_elfpath = proc_ptr.dereference()['p_elfpath']
        return p_elfpath


def get_stop_handler(elves):

    def stop_handler(event):
        user_elf_base_addr = 0x00400000
        kernel_base = 0x80000000    
    
        pc = gdb.parse_and_eval('$pc')
        pc = int(pc) &  0xffffffff
    
        in_kernel_mode = pc >= kernel_base

        mimiker_path = get_p_elfpath()

        if mimiker_path == None:
            return
    
        print(f'mimiker_path = {mimiker_path}')
        mimiker_path = str(mimiker_path).split(',', 1)[0][1:-1] 

        try:
            host_path = elves[mimiker_path]
        except:
            print(f'no {mimiker_path} in elves dict')
            return

        try:
            gdb.execute(
                f'remove-symbol-file -a {hex(user_elf_base_addr)}')
        except:
            print(f"no symbol file loaded at {hex(user_elf_base_addr)}")
        

        try: 
            gdb.execute(f'add-symbol-file  {host_path}')
        except:
            print(f'no symbol file {host_path}')

    return stop_handler
