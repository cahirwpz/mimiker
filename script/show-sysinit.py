#!/usr/bin/env python3

import os
import re
import itertools


def listdir_path(d):
    return [os.path.join(d, f) for f in os.listdir(d)]


if __name__ == "__main__":
    sysinit = re.compile(r'SYSINIT_ADD\((.*), .*, (?:NODEPS|DEPS\((.*)\))\)')
    #             matches first argument/\   matches last argument/\
    print('digraph mimiker_modules {')
    folders = ["sys", "mips"]
    for source in itertools.chain.from_iterable(map(listdir_path, folders)):
        try:
            with open(source, 'r', encoding='ascii') as f:
                matches = sysinit.findall(f.read())
                for match in matches:
                    module = match[0]
                    print("{};".format(module))
                    deps = match[1].split(', ')
                    deps = [d.lstrip('"').rstrip('"') for d in deps]
                    print("{} -> {{{}}};".format(module, " ".join(deps)))

        except UnicodeDecodeError:
            continue
    print("}")
