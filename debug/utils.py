import gdb
import re
import subprocess32 as subprocess


class Vars():

    @staticmethod
    def global_vars():
        output = subprocess.check_output(['mipsel-mimiker-elf-readelf',
                                          '-sW', 'mimiker.elf'],
                                         universal_newlines=True)

        lines = re.findall('^\s*\S+\s+\S+\s+\S+\s+\S+'
                           '\s+\S+\s+\S+\s+\S+\s+\S+$',
                           output, re.MULTILINE)

        result = []
        for line in lines[1:]:
            _, _, _, _, _, _, _, varname = line.split()
            result.append(varname)

        return result

    @staticmethod
    def local_vars():
        decls_string = gdb.execute('info locals', False, True)
        return re.findall('^\S+', decls_string, re.MULTILINE)

    @staticmethod
    def get_typename_of(var):
        output = gdb.execute('whatis %s' % var, False, True)
        return output[7:-1]

    @staticmethod
    def has_type(var, typename):
        try:
            typename_of_var = Vars.get_typename_of(var)
        except:
            return False
        type_decl = Vars.__remove_spaces(typename_of_var)
        return (Vars.__remove_spaces(typename) == type_decl)

    @staticmethod
    def __remove_spaces(typename):
        splt = typename.split(' ')
        return reduce((lambda x, y: x+y), splt)


def cast(value, typename):
    return value.cast(gdb.lookup_type(typename))


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
        m = m.groups()
        return '%s:%s' % (m[1], m[0])


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


class GdbStructBase(object):
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

    @property
    def address(self):
        return str(self._obj.address)


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
        return super(GdbStructMeta, cls).__new__(
                cls, name, (GdbStructBase,) + bases, dct)
