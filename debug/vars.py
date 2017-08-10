import gdb
import subprocess


def global_vars():
    output = subprocess.check_output(['mipsel-mimiker-elf-readelf',
                                      '-sW', 'mimiker.elf'],
                                     universal_newlines=True)

    records = [line.strip().split() for line in output.splitlines()[3:]]
    return [fields[7] for fields in records if len(fields) == 8]


def local_vars():
    decls_string = gdb.execute('info locals', False, True)
    return [decl.split()[0] for decl in decls_string.splitlines()[:-1]]


def get_typename_of(var):
    output = gdb.execute('whatis %s' % var, False, True)
    return output[7:-1]


def has_type(var, typename):
    try:
        typename_of_var = get_typename_of(var)
    except:
        return False
    type_decl = _remove_spaces(typename_of_var)
    return (_remove_spaces(typename) == type_decl)


def _remove_spaces(typename):
    splt = typename.split(' ')
    return reduce((lambda x, y: x+y), splt)
