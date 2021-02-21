import gdb
import re
from .utils import cast, relpath


def cstr(val):
    try:
        return val.string()
    except gdb.MemoryError:
        return '[bad-ptr 0x%x]' % val.address


def enum(v):
    return v.type.target().fields()[int(v)].name


class ProgramCounter():
    def __init__(self, pc):
        self.pc = cast(pc, 'unsigned long')

    def __str__(self):
        if self.pc == 0:
            return 'null'
        line = gdb.execute('info line *0x%x' % self.pc, to_string=True)
        m = re.match(r'Line (\d+) of "(.*)"', line)
        if m:
            lnum, path = m.groups()
            return '%s:%s' % (relpath(path), lnum)
        else:
            return '0x%x' % self.pc


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
                def _caster(x):
                    val = x._obj[fname]
                    try:
                        return caster(val)
                    except gdb.MemoryError:
                        return '[bad-ptr: 0x%x]' % val.address
                return _caster
            caster = None
            if '__cast__' in dct:
                caster = dct['__cast__'].get(f.name, None)
            dct[f.name] = property(mkgetter(f.name, caster))
        # classes created with GdbStructMeta will inherit from GdbStructBase
        return super().__new__(cls, name, (GdbStructBase,) + bases, dct)


class BinTime(metaclass=GdbStructMeta):
    __ctype__ = 'struct bintime'
    __cast__ = {'sec': int, 'frac': int}

    def as_float(self):
        return float(self.sec) + float(self.frac) / 2**64

    def __str__(self):
        return 'bintime{%.6f}' % self.as_float()


class List():
    def __init__(self, lst, field):
        self.lst = lst
        self.field = field

    def __iter__(self):
        item = self.lst['lh_first']
        while item != 0:
            item = item.dereference()
            yield item
            item = item[self.field]['le_next']


class TailQueue():
    def __init__(self, tq, field):
        self.tq = tq
        self.field = field

    def __iter__(self):
        item = self.tq['tqh_first']
        while item != 0:
            item = item.dereference()
            yield item
            item = item[self.field]['tqe_next']


class LinkerSet():
    def __init__(self, name, typ):
        self.start = gdb.parse_and_eval('(%s **)&__start_set_%s' % (typ, name))
        self.stop = gdb.parse_and_eval('(%s **)&__stop_set_%s' % (typ, name))

    def __iter__(self):
        item = self.start
        while item < self.stop:
            yield item.dereference().dereference()
            item = item + 1
