#!/usr/bin/env python3

import os.path
import sys
import subprocess


ASSYM_BIAS = 0x10000000


if __name__ == "__main__":
    nm = sys.argv[1]
    infile = os.path.abspath(sys.argv[2])
    outfile = os.path.abspath(sys.argv[3])
    symtab = subprocess.check_output([nm, infile]).decode()
    symbols = [s.split() for s in symtab.strip().split('\n')]
    with open(outfile, 'w') as output:
        for symbol in symbols:
            try:
                value, _, name = symbol
            except ValueError:
                continue
            name = name.rstrip('$')
            value = int(value, 16) - ASSYM_BIAS
            print("#define", name, "0x%x" % value, file=output)
