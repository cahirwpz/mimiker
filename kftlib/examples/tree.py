#!/usr/bin/env python3
import argparse

from pathlib import Path

from kftlib.elf import Elf
from kftlib.event import KFTEventType
from kftlib import inspect


def get_fn_graphs(events, elf, function, max_depth):
    graphs = []
    function_pc = elf.fun2pc[function]
    for td, td_events in events.items():
        graph = ""
        depth = 0
        start_time = 0
        drawing_graph = False
        for event in td_events:
            if not drawing_graph and event.pc != function_pc:
                continue

            if event.pc == function_pc and event.typ == KFTEventType.KFT_IN:
                drawing_graph = True
                start_time = event.timestamp
                graph = ""

            append = not max_depth or depth <= max_depth

            time = event.timestamp - start_time
            if event.typ == KFTEventType.KFT_IN:
                fn_name = elf.pc2fun[event.pc]
                line = f"{time:>4}  " + depth * "| " + fn_name + "\n"  # in
                depth += 1
            else:
                depth -= 1
                line = f"{time:>4}  " + depth * "| " + "*" + "\n"  # out

            if append:
                graph += line

            if event.pc == function_pc and event.typ == KFTEventType.KFT_OUT:
                graphs.append((time, graph))
                drawing_graph = False

    # Sort by time and return only graphs
    graphs.sort()
    _, graphs = zip(*graphs)
    return graphs


def main():
    parser = argparse.ArgumentParser(
        description="Launch kernel in a board simulator."
    )
    parser.add_argument("function",
                        type=str,
                        help="Function to show call graph for.")
    parser.add_argument("kft_dump",
                        type=Path,
                        default=Path("dump.kft"),
                        help="KFTrace dump to process")
    parser.add_argument("-e", "--elf-file",
                        type=Path,
                        default=Path("sys/mimiker.elf"),
                        help="Path to mimiker.elf")
    parser.add_argument("-d", "--max-depth",
                        type=int)
    parser.add_argument("-o", "--out",
                        type=Path,
                        default=Path("graph.txt"))

    args = parser.parse_args()
    elf = Elf.inspect_elf_file(args.elf_file)
    events = inspect.inspect_kft_file(args.kft_dump, elf)
    graphs = get_fn_graphs(events, elf, args.function, args.max_depth)
    with open(args.out, "w") as f:
        f.write("\n\n".join(graphs))


if __name__ == "__main__":
    main()
