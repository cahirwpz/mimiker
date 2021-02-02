import gdb
import os
import os.path
import re
import shutil
import texttable


def cast(value, typename):
    return value.cast(gdb.lookup_type(typename))


def local_var(name):
    return gdb.newest_frame().read_var(name)


def global_var(name):
    return gdb.parse_and_eval(name)


def relpath(path):
    cwd = os.path.dirname(gdb.current_progspace().filename)
    if path.startswith(cwd):
        n = len(cwd) + 1
        path = path[n:]
    return path


# calculates address of ret instruction within function body (MIPS specific)
def func_ret_addr(name):
    s = gdb.execute('disass thread_create', to_string=True)
    for line in s.split('\n'):
        m = re.match(r'\s+(0x[0-9a-f]{8})\s+<\+\d+>:\tjr\tra', line)
        if m:
            return int(m.groups()[0], 16)


class TextTable(texttable.Texttable):
    termsize = shutil.get_terminal_size(fallback=(80, 25))

    def __init__(self, types=None, align=None):
        super().__init__(self.termsize[0])
        self.set_deco(self.HEADER | self.VLINES | self.BORDER)
        if types:
            self.set_cols_dtype(types)
        if align:
            self.set_cols_align(align)

    def __str__(self):
        return self.draw()
