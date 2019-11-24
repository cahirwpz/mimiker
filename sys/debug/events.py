import gdb
from pathlib import Path
from debug.proc import Process


def get_current_mimiker_path():
    try:
        return Process.from_current().p_elfpath.string()
    except:  # noqa: E722
        return None


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
