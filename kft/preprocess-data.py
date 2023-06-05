#!/usr/bin/env python3

import os
import pickle
import sys

from tqdm import tqdm

from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection


PC_MASK = 0xFFFFFF           # 24 bits
TIMESTAMP_MASK = 0x7FFFFFFF  # 31 bits
THREAD_MASK = 0xF            # 8 bits
TYPE_MASK = 0x1              # 1 bit

PC_SHIFT = 40
TIMESTAMP_SHIFT = 9
THREAD_SHIFT = 1
TYPE_SHIFT = 0

KFT_IN = 0
KFT_OUT = 1


# Returns:
#   - kernel_start
#   - dict mapping pc to function
#   - dict mapping function to pc
def inspect_elf_file(elf_file):
    def get_symbol_table_section(elf):
        for section in elf.iter_sections():
            if not isinstance(section, SymbolTableSection):
                continue
            return section

    def is_function(s):
        return s.entry['st_info']['type'] == 'STT_FUNC'

    def read_symbol(s):
        return (s.entry['st_value'], s.name)

    with open(elf_file, 'rb') as elf_file:
        elf = ELFFile(elf_file)

        sym_table = get_symbol_table_section(elf)
        if sym_table is None:
            return None, None, None

        kern_sym = sym_table.get_symbol_by_name('__kernel_start')
        if kern_sym is None:
            kern_start = None
        else:
            kern_start = read_symbol(kern_sym[0])[0]

        pc_to_fun = [read_symbol(s)
                     for s in sym_table.iter_symbols() if is_function(s)]
        pcs, fns = zip(*pc_to_fun)
        fun_to_pc = zip(fns, pcs)
        pc2fun = dict(pc_to_fun)
        fun2pc = dict(fun_to_pc)

        return kern_start, pc2fun, fun2pc


def decode_values(v, kern_start):
    typ = (v >> TYPE_SHIFT) & TYPE_MASK
    thread = (v >> THREAD_SHIFT) & THREAD_MASK
    timestamp = (v >> TIMESTAMP_SHIFT) & TIMESTAMP_MASK
    rel_pc = (v >> PC_SHIFT) & PC_MASK
    pc = kern_start + rel_pc
    return thread, typ, timestamp, pc


# Read events and split them into threads
# Return dict of events for threads
def inspect_kft_file(path, kern_start):
    st = os.stat(path)
    size = st.st_size
    entries = int(size / 8)
    print(f'Reading file of size {size/1024/1024:.2f}MB ({entries} entries)')

    MAX_THREADS = 30
    events = dict()
    td_time = [0] * MAX_THREADS  # elapsed time
    cur_thread = -1
    cur_time = 0
    switch_time = 0
    ctx_swith_count = 0

    file = open(path, 'rb', buffering=800)
    for i in tqdm(range(entries), ncols=80):
        v = int.from_bytes(file.read(8), 'little')
        thread, typ, tm, pc = decode_values(v, kern_start)

        if thread != cur_thread:
            # update info about prev thread
            td_time[cur_thread] += tm - switch_time

            if thread not in events:
                events[thread] = []

            # save values for current thread
            switch_time = tm
            cur_time = td_time[thread]
            cur_thread = thread
            ctx_swith_count += 1

        timestamp = cur_time + (tm - switch_time)

        # Append tuple in correct order (type, timestampt, pc)
        events[cur_thread].append((typ, timestamp, pc))

    print(f'Encountered {ctx_swith_count} context swithces')
    return events


def trunc_opener(path, flags):
    return os.open(path, flags | os.O_TRUNC, mode=0o664)


def dump_threads(dir, td_events):
    for t in td_events.keys():
        filename = dir + f'/thread-{t}.pkl'
        print(f'Writing thread {t} data to {filename}.')
        with open(filename, 'wb', opener=trunc_opener) as file:
            pickle.dump(td_events[t], file)


def dump_functions(dir, pc2fun, fun2pc):
    pc_filename = dir + '/pc2fun.pkl'
    fn_filename = dir + '/fun2pc.pkl'

    print(f'Writing pc2fun dict to {pc_filename}')
    with open(pc_filename, 'wb', opener=trunc_opener) as file:
        pickle.dump(pc2fun, file)

    print(f'Writing fun2pc dict to {fn_filename}')
    with open(fn_filename, 'wb', opener=trunc_opener) as file:
        pickle.dump(fun2pc, file)


def main():
    if len(sys.argv) < 3:
        print("provide a kft file and output directory as arguments!")
        sys.exit(1)

    # Prepare arguments
    path = sys.argv[1]
    dir = sys.argv[2]
    elf_file = 'sys/mimiker.elf'
    print(f'inspecting file `{path}` using symbols from `{elf_file}`')

    # Create output directory
    try:
        os.stat(dir)
        choice = input(f'Path `{dir}`exists do you want to continue (Y/n) ')
        if choice not in {'y', 'Y', 'yes', 'Yes'}:
            print("Aborting.")
            sys.exit(0)
    except FileNotFoundError:
        os.mkdir(dir)

    # Inspect ELF file
    kern_start, pc2fun, fun2pc = inspect_elf_file(elf_file)
    if pc2fun is None:
        print("symbols not found in elf file!")
        sys.exit(1)
    if kern_start is None:
        print(f'`__kernel_start` symbol not found in {elf_file}')
        sys.exit(1)

    # print(f'kernel start: {hex(kern_start)}')

    # Inspect KFT output
    td_events = inspect_kft_file(path, kern_start)

    # Dump data to files for later use
    dump_functions(dir, pc2fun, fun2pc)
    dump_threads(dir, td_events)


if __name__ == '__main__':
    main()
