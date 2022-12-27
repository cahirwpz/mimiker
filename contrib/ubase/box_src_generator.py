#!/usr/bin/env python3

import argparse

from contextlib import redirect_stdout


def generate_core_file(program: str, tools: list):
    main_prefixes = [tool.replace('-', '_') for tool in tools]

    with open(program + '.c', 'w') as f:
        with redirect_stdout(f):
            print('#include <libgen.h>')
            print('#include <stdio.h>')
            print('#include <stdlib.h>')
            print('#include <string.h>')
            print('#include "util.h"')

            for prefix in main_prefixes:
                print(f'int {prefix}_main(int, char**);')

            print('int main(int argc, char *argv[]) { \
                char *s = basename(argv[0]);')
            print(f'if(!strcmp(s,\"{program}\")) {{ \
                argc--; argv++; s = basename(argv[0]); }} if(0) ;')

            for tool, prefix in zip(tools, main_prefixes):
                print(f'else if(!strcmp(s, \"{tool}\")) \
                    return {prefix}_main(argc, argv);')

            print('else {')
            for tool in tools:
                print(f'fputs(\"{tool} \", stdout);')
            print('putchar(0xa); }; return 0; }')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Generate ubase-box core source file.')
    parser.add_argument('-p', '--program', type=str,
                        help='Executable file name.')
    parser.add_argument('-t', '--tools', nargs='+',
                        help='List of supported tools.')

    args = parser.parse_args()

    generate_core_file(args.program, args.tools)
