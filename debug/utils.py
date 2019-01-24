import gdb
import os
import re
import shutil
import traceback
import texttable


def print_exception(func):
    def wrapper(*args, **kwargs):
        try:
            return func(*args, **kwargs)
        except:
            traceback.print_exc()
    return wrapper


def cast(value, typename):
    return value.cast(gdb.lookup_type(typename))


def cstr(val):
    return val.string()


def enum(v):
    return v.type.target().fields()[int(v)].name


def local_var(name):
    return gdb.newest_frame().read_var(name)


def global_var(name):
    return gdb.parse_and_eval(name)


def relpath(path):
    cwd = os.getcwd()
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


class ProgramCounter():
    def __init__(self, pc):
        self.pc = cast(pc, 'unsigned long')

    def __str__(self):
        if self.pc == 0:
            return 'null'
        line = gdb.execute('info line *0x%x' % self.pc, to_string=True)
        m = re.match(r'Line (\d+) of "(.*)"', line)
        lnum, path = m.groups()
        return '%s:%s' % (relpath(path), lnum)


class OneArgAutoCompleteMixin():
    def options(self):
        raise NotImplementedError

    def complete_rest(self, first, text, word):
        return []

    # XXX: Completion does not play well when there are hypens in keywords.
    def complete(self, text, word):
        options = list(self.options())
        try:
            first, rest = text.split(' ', 1)
        except ValueError:
            return [option for option in options
                    if option.startswith(text) and text != option]
        return self.complete_rest(first, rest, word)


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


class UserCommand():
    def __init__(self, name):
        self.name = name

    def __call__(self, args):
        raise NotImplementedError


class TraceCommand(UserCommand):
    def __init__(self, name, breakpoint):
        super().__init__(name)
        self.__instance = None
        self.breakpoint = breakpoint

    def __call__(self, args):
        if self.__instance:
            print('{} deactivated!'.format(self.__doc__))
            self.__instance.delete()
            self.__instance = None
        else:
            print('{} activated!'.format(self.__doc__))
            self.__instance = self.breakpoint()


class CommandDispatcher(gdb.Command, OneArgAutoCompleteMixin):
    def __init__(self, name, commands):
        assert all(isinstance(cmd, UserCommand) for cmd in commands)

        self.name = name
        self.commands = {cmd.name: cmd for cmd in commands}
        super().__init__(name, gdb.COMMAND_USER)

    def list_commands(self):
        return '\n'.join('{} {} -- {}'.format(self.name, name, cmd.__doc__)
                         for name, cmd in sorted(self.commands.items()))

    @print_exception
    def invoke(self, args, from_tty):
        try:
            cmd, args = args.split(' ', 1)
        except ValueError:
            cmd, args = args, ''

        if not cmd:
            raise gdb.GdbError('{}\n\nList of commands:\n\n{}'.format(
                               self.__doc__, self.list_commands()))

        if cmd not in self.commands:
            raise gdb.GdbError('No such subcommand "{}"'.format(cmd))

        try:
            self.commands[cmd](args)
        except:
            traceback.print_exc()

    def options(self):
        return list(self.commands.keys())
