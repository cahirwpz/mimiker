import gdb


def get_p_elfpath():
    pcpu_data = gdb.newest_frame().read_var('_pcpu_data')
    curthread_ptr = pcpu_data.dereference()['curthread']
    proc_ptr = curthread_ptr.dereference()['td_proc']
    if proc_ptr == 0:
        return None
    p_elfpath = proc_ptr.dereference()['p_elfpath']
    return p_elfpath.string()


def mimiker_path_to_host_path(p_elfpath):
    return f'sysroot{p_elfpath}.dbg'


def get_stop_handler():

    def stop_handler(event):
        #user_elf_base_addr = 0x00400000
        #kernel_base = 0x80000000

        #pc = gdb.parse_and_eval('$pc')
        #pc = int(pc) & 0xffffffff

        #in_kernel_mode = pc >= kernel_base

        #mimiker_path = get_p_elfpath()
        #print(mimiker_path)

        if mimiker_path is None:
            return

        host_path = mimiker_path_to_host_path(mimiker_path)

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
