#!/usr/bin/env python3

"""
Example application using kftlib.
"""

import argparse
import statistics
import os
from kftlib.elf import Elf
from kftlib import inspect
from kftlib.stats import get_functions_times

from pathlib import Path


def get_fn_times(events, elf, functions, out):
    fn_pcs = list(map(lambda fn: elf.fun2pc.get(fn), functions))
    fn_times = get_functions_times(events, fn_pcs)

    sumt = 0
    print(f"{'function':>13}: {'count':>5} {'avg time':>8}")
    for fn, pc in zip(functions, fn_pcs):
        if not pc:
            continue
        if pc in fn_times:
            avg_time = statistics.mean(fn_times[pc])
            count = len(fn_times[pc])
            print(f"{fn:>13}: {count:>5} {avg_time:>8.0f}")
            if fn in ["vm_map_clone", "vm_page_fault"]:
                sumt += sum(fn_times[pc])
            if fn in ["pmap_protect"]:
                sumt -= sum(fn_times[pc])

    print("SUM:", sumt)

    os.makedirs(out, exist_ok=True)
    for fn, pc in zip(functions, fn_pcs):
        if not pc:
            continue
        with open(f"{out}/{fn}.data", "w") as f:
            f.write("\n".join(str(t) for t in fn_times[pc]) + '\n')


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
    parser.add_argument("-o", "--out",
                        type=Path,
                        default=Path("plot-data"))
    args = parser.parse_args()

    elf = Elf.inspect_elf_file(args.elf_file)
    events = inspect.inspect_kft_file(args.kft_dump, elf)

    functions = [
        "vm_amap_find_anon",
        "vm_object_find_page",
        "vm_page_fault",
        "pmap_protect",
        "pmap_copy_page",
        "vm_page_alloc",
        "vm_map_clone",
        "vm_amap_alloc",
        "vm_object_alloc",
    ]

    get_fn_times(events, elf, functions, args.out)


if __name__ == "__main__":
    main()
