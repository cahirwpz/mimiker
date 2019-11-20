import gdb
from pathlib import Path


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


def loaded_current_paths():
    loaded_host_path = gdb.objfiles()[1].filename
    current_mimiker_path = get_p_elfpath()
    if current_mimiker_path is None:
        current_host_path = None
    else:
        current_host_path = mimiker_path_to_host_path(current_mimiker_path)
    return loaded_host_path, current_host_path


def get_stop_handler():

    def stop_handler(event):
        user_elf_base_addr = 0x400000
        loaded_host_path, current_host_path = loaded_current_paths()
        if current_host_path is None:
            return
        current_host_path = str(Path(current_host_path).resolve())
        if loaded_host_path != current_host_path:
            try:
                gdb.execute(
                    f'remove-symbol-file -a {hex(user_elf_base_addr)}')
                gdb.execute(f'add-symbol-file  {current_host_path}', to_string=True)
            except:
                print('failed to swap symbol files')

    return stop_handler
