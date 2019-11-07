import gdb

def stop_handler(event):
    user_elf_base_addr = 0x00400000
    
    pc = gdb.parse_and_eval('$pc')

    if not pc < 0x80000000:
        return; 

    try:
        gdb.execute(
            f'remove-symbol-file -a {hex(user_elf_base_addr)}')
    except:
        print("no symbol file loaded at 0x40000")
        pass
    
    mimiker_path = gdb.parse_and_eval('$process()->p_elfpath')
    print(mimiker_path)
    host_path = "bin/cat/cat.uelf"

    gdb.execute(
        f'add-symbol-file  {host_path}')
    
