#!/usr/bin/env python3

import argparse
import pickle
import sys

from tqdm import tqdm

# Accessors for KFT event tuple
TYPE = 0
TIME = 1
PC = 2

KFT_IN = 0
KFT_OUT = 1


class KFT():
    def __init__(self, events, pc2fun, fun2pc):
        self.events = events
        self.pc2fun = pc2fun
        self.fun2pc = fun2pc

        self.stats = dict()

    def print_graph(self, file):
        depth = 0
        time_stack = []
        bar = '│ '

        for event in tqdm(self.events, ncols=80):
            time_str = ''
            fn_name = ''
            if event[TYPE] == KFT_OUT:
                depth -= 1
                elapsed = event[TIME] - time_stack.pop()
                time_str = f'(ticks: {elapsed})'

            if depth < 0:
                print("Errororrr!")
                sys.exit(1)

            if event[TYPE] == KFT_IN:
                fn_name = self.pc2fun[event[PC]]

            line = f'{event[TIME]:06d} {bar * depth}{fn_name}{time_str}'
            file.write(line + '\n')

            if event[TYPE] == KFT_IN:
                depth += 1
                time_stack.append(event[TIME])

    # for list of function pcs get list of their invocation times
    def _get_stats(self, pcs):
        res = dict([(pc, Stat(pc, self.pc2fun[pc])) for pc in pcs])
        last_time = dict.fromkeys(pcs, -1)

        for pc in pcs:
            res[pc].times = []
            res[pc].count = 0

        for event in self.events:
            pc = event[PC]
            if pc not in pcs:
                continue

            t_last = last_time[pc]
            if event[TYPE] == KFT_IN:
                assert t_last == -1
                last_time[pc] = event[TIME]
            else:
                assert t_last != -1
                tm = event[TIME] - t_last
                last_time[pc] = -1
                res[pc].times.append(tm)
                res[pc].count += 1

        return res

    # display time info for given functions
    # if fn_pcs is True then list contains pc values
    def get_stats(self, fn_list, fn_pcs=False):
        invalid = []
        for fn in fn_list:
            if ((fn_pcs and fn not in self.pc2fun.keys())
                    or (fn not in self.fun2pc.keys())):
                invalid.append(fn)
        for i in invalid:
            fn_list.remove(i)

        if not fn_pcs:
            pc_list = [self.fun2pc[f] for f in fn_list]
        else:
            pc_list = fn_list

        res = dict()
        todo = []

        for i in range(len(fn_list)):
            fn = fn_list[i]
            pc = pc_list[i]

            if fn in invalid:
                print(f'Function {fn} is not valid')
                continue

            if pc in self.stats:
                res[fn] = self.stats[pc]
            else:
                todo.append(pc)

        if len(todo) > 0:
            st = self._get_stats(todo)
            for pc, s in st.items():
                self.stats[pc] = s
                res[pc] = s

        return res


class Stat:
    def __init__(self, pc, name):
        self.pc = pc
        self.name = name
        self.times = []  # dict of list of times for each thread
        self.count = 0   # dict of count for each thread

    def __repr__(self):
        return f'Stat(pc={hex(self.pc)}, fn={self.name})'

    def display(self):
        if len(self.times) != 0:
            av = sum(self.times) / len(self.times)
            mx = max(self.times)
            mi = min(self.times)
        else:
            av = -1
            mx = -1
            mi = -1

        print(f'{self.name} ({hex(self.pc)}) stats:')
        print(f'\tinvocations: {self.count}')
        print(f'\tavg time: {av}')
        print(f'\tmax time: {mx}')
        print(f'\tmin time: {mi}')


def get_data(path):
    with open(path, 'rb') as file:
        return pickle.load(file)


# Expecting directory structure:
# - pc2fun.pickle
# - fun2pc.pickle
# - thread-i.pickle (for i \in Naturals)
def main():
    parser = argparse.ArgumentParser(
        description='Analyze data gathered from kernel by KFT.')

    parser.add_argument('-d', '--dir', type=str,
                        help='Directory with KFT data preprocessed')
    parser.add_argument('-g', '--graph', action='store_true',
                        help='Print function call graph')
    parser.add_argument('-t', '--thread', type=int,
                        help='Id of thread to inspect')
    args = parser.parse_args()

    thread = args.thread
    dir = args.dir

    pc2fun = get_data(dir + '/pc2fun.pkl')
    fun2pc = get_data(dir + '/fun2pc.pkl')

    events = get_data(dir + f'/thread-{thread}.pkl')

    kft = KFT(events, pc2fun, fun2pc)

    if args.graph:
        filename = dir + f'/thread-{thread}.graph'
        with open(filename, 'w') as file:
            print(f'Writing function call graph to `{filename}`')
            kft.print_graph(file)

    inspect_functions = ['vm_page_fault', 'do_fork', 'do_mmap', 'do_blablabla']
    stats = kft.get_stats(inspect_functions)

    inspect_functions.extend(['do_exec', 'do_exit', 'proc_create'])
    stats = kft.get_stats(inspect_functions)

    for p in stats.values():
        p.display()


if __name__ == '__main__':
    main()
