import gdb
from pathlib import Path


def get_current_mimiker_path():
    pcpu_data = gdb.newest_frame().read_var('_pcpu_data')
    curthread_ptr = pcpu_data.dereference()['curthread']
    proc_ptr = curthread_ptr.dereference()['td_proc']
    if proc_ptr == 0:
        return None
    p_elfpath = proc_ptr.dereference()['p_elfpath']
    if p_elfpath == 0:
        return None
    return p_elfpath.string()


def get_loaded_host_path():
    try:
        return gdb.objfiles()[1].filename
    except:  # noqa: E722
        return None


def mimiker_path_to_host_path(p_elfpath):
    return None if p_elfpath is None else f'sysroot{p_elfpath}.dbg'


def stop_handler(event):
    user_elf_base_addr = 0x400000
    loaded_host_path = get_loaded_host_path()
    current_host_path = mimiker_path_to_host_path(get_current_mimiker_path())

    if current_host_path is None:
        return

    current_host_path = str(Path(current_host_path).resolve())
    if loaded_host_path != current_host_path:
        try:
            gdb.execute(
                f'remove-symbol-file -a {hex(user_elf_base_addr)}',
                to_string=True
            )
            gdb.execute(
                f'add-symbol-file  {current_host_path}', to_string=True)
        except:  # noqa: E722
            print('failed to swap symbol files')
