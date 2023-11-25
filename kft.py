#!/usr/bin/env python3

"""
Example application using kftlib.
"""

import argparse
import kftlib

from pathlib import Path


def get_fn_times(events, elf):
    functions = [
        "vm_map_clone",
        "do_fork",
        "vm_page_fault",
    ]

    fn_pcs = list(map(lambda fn: elf.fun2pc[fn], functions))
    fn_times = kftlib.get_functions_times(events, fn_pcs)

    for fn, pc in zip(functions, fn_pcs):
        print(f"{fn}: {len(fn_times[pc])}")


def main():
    parser = argparse.ArgumentParser(
        description="Launch kernel in a board simulator.")
    parser.add_argument("kft_dump",
                        type=Path,
                        default=Path("dump.kft"),
                        help="KFTrace dump to process")
    parser.add_argument("-e", "--elf-file",
                        type=Path,
                        default=Path("sys/mimiker.elf"),
                        help="Path to mimiker.elf")
    args = parser.parse_args()

    elf = kftlib.Elf.inspect_elf_file(args.elf_file)
    events = kftlib.inspect_kft_file(args.kft_dump, elf)

    get_fn_times(events, elf)


if __name__ == "__main__":
    main()
