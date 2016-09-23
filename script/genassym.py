#!/usr/bin/env python3

import os.path
import sys
import subprocess

if __name__ == "__main__":
    nm = sys.argv[1]
    infile = os.path.abspath(sys.argv[2])
    outfile = os.path.splitext(infile)[0] + '.h'
    symtab = subprocess.check_output([nm, infile]).decode()
    symbols = [s.split() for s in symtab.strip().split('\n')]
    with open(outfile, 'w') as output:
        for value, _, name in symbols:
            name = name.rstrip('$')
            print("#define", name, "0x%x" % int(value, 16), file=output)
