#!/usr/bin/env python3

import argparse

from contextlib import redirect_stdout


def generate_ubase_box_source(program: str, tools: list):
    main_prefixes = [tool.replace('-', '_') for tool in tools]

    print('#include <libgen.h>')
    print('#include <stdio.h>')
    print('#include <stdlib.h>')
    print('#include <string.h>')
    print('#include "util.h"')

    # needed for su and login
    print('char* envinit[1];')

    for prefix in main_prefixes:
        print(f'int {prefix}_main(int, char**);')

    print('')
    print('int main(int argc, char *argv[]) {')
    print('  char *s = basename(argv[0]);')
    print(f'  if(!strcmp(s,\"{program}\")) {{')
    print('    argc--;')
    print('    argv++;')
    print('    s = basename(argv[0]);')
    print('  }')

    for tool, prefix in zip(tools, main_prefixes):
        print(f'  if(!strcmp(s, \"{tool}\"))')
        print(f'    return {prefix}_main(argc, argv);')

    for tool in tools:
        print(f'  fputs(\"{tool} \", stdout);')
    print('  putchar(0xa);')
    print('  return 0;')
    print('}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Generate ubase-box source file.')
    parser.add_argument('-p', '--program', type=str,
                        help='Executable file name.')
    parser.add_argument('-t', '--tools', nargs='+',
                        help='List of supported tools.')

    args = parser.parse_args()

    with open(args.program + '.c', 'w') as f:
        with redirect_stdout(f):
            generate_ubase_box_source(args.program, args.tools)
