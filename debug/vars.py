import gdb
from subprocess import check_output
from collections import namedtuple

"""This simple module allows reading local/global variable names and
checking their types. It operates on textual representation of types,
which is awful an in general incorrect, but is the only option since
GDB API has severe limitations. E.g. the canonical impementation of
`has_type(var, typename)` would be first to convert `var` to `Value`
object, and then compare its `type` field with the result of
`gdb.lookup_type(typename)`. However, the latter command simply fails
when `typename` denotes a pointer type and therefore cannot be used.
"""

ReadelfRow = namedtuple('ReadelfRow',
                        'num value size type '
                        'bind vis ndx name')


def set_of_globals():
    """ Return set of global variable names."""
    def is_variable(name):
        try:
            get_typename_of(name)
        except:
            return False
        return True

    output = check_output(['mipsel-mimiker-elf-readelf',
                           '-sW', 'mimiker.elf'],
                          universal_newlines=True)

    records = [line.strip().split() for line in output.splitlines()[3:]]
    tuples = [ReadelfRow(*fields) for fields in records if len(fields) == 8]
    return set(fields.name for fields in tuples
               if (fields.type == 'OBJECT') and is_variable(fields.name))


def set_of_locals():
    """ Return set of local variable names."""
    current_frame = gdb.selected_frame()
    current_block = current_frame.block()

    result = set(str(symbol) for symbol in current_block
                 if symbol.is_variable or symbol.is_argument)

    while current_block.function is None:
        current_block = current_block.superblock
        result.update(str(symbol) for symbol in current_block
                      if symbol.is_variable or symbol.is_argument)

    return result


def get_typename_of(var):
    """ Return a string being a textual representation of var's type."""
    object = gdb.parse_and_eval(var)
    return str(object.type)


def has_type(var, typename):
    """ Checks if var's type match typename. Argument typename is a string."""
    typename_of_var = get_typename_of(var)

    type_decl = _remove_spaces(typename_of_var)
    return (_remove_spaces(typename) == type_decl)


def _remove_spaces(typename):
    return typename.replace(' ', '')
