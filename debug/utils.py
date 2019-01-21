import gdb
import os
import re


def cast(value, typename):
    return value.cast(gdb.lookup_type(typename))


def enum(v):
    return v.type.target().fields()[int(v)].name


def local_var(name):
    return gdb.newest_frame().read_var(name)


# calculates address of ret instruction within function body (MIPS specific)
def func_ret_addr(name):
    s = gdb.execute('disass thread_create', to_string=True)
    for line in s.split('\n'):
        m = re.match(r'\s+(0x[0-9a-f]{8})\s+<\+\d+>:\tjr\tra', line)
        if m:
            return int(m.groups()[0], 16)


class ProgramCounter():
    def __init__(self, pc):
        self.pc = cast(pc, 'unsigned long')

    def __str__(self):
        if self.pc == 0:
            return 'null'
        line = gdb.execute('info line *0x%x' % self.pc, to_string=True)
        m = re.match(r'Line (\d+) of "(.*)"', line)
        lnum, path = m.groups()
        cwd = os.getcwd()
        if path.startswith(cwd):
            n = len(cwd) + 1
            path = path[n:]
        return '%s:%s' % (path, lnum)


class OneArgAutoCompleteMixin():
    def options(self):
        raise NotImplementedError

    def complete(self, text, word):
        args = text.split()
        options = self.options()
        if len(args) == 0:
            return options
        if len(args) >= 2:
            return []
        suggestions = []
        for o in options:
            if o.startswith(args[0], 0, len(o) - 1):
                suggestions.append(o)
        return suggestions


class GdbStructBase():
    def __init__(self, obj):
        self._obj = obj

    def to_string(self):
        return str(self)

    def dump(self):
        res = ['%s = %s' % (field, getattr(self, field))
               for field in self._obj.type]
        return '\n'.join(res)

    def display_hint(self):
        return 'map'


class GdbStructMeta(type):
    def __new__(cls, name, bases, dct):
        t = gdb.lookup_type(dct['__ctype__'])
        # for each field of ctype make property getter of the same name
        for f in t.fields():
            def mkgetter(fname, caster):
                if caster is None:
                    return lambda x: x._obj[fname]
                # use cast function if available
                return lambda x: caster(x._obj[fname])
            caster = None
            if '__cast__' in dct:
                caster = dct['__cast__'].get(f.name, None)
            dct[f.name] = property(mkgetter(f.name, caster))
        # classes created with GdbStructMeta will inherit from GdbStructBase
        return super().__new__(cls, name, (GdbStructBase,) + bases, dct)
