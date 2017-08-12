import gdb
import subprocess
import collections


def list_of_globals():
    def is_variable(name):
        try:
            get_typename_of(name)
        except:
            return False
        return True

    output = subprocess.check_output(['mipsel-mimiker-elf-readelf',
                                      '-sW', 'mimiker.elf'],
                                     universal_newlines=True)

    ReadelfRow = collections.namedtuple('ReadelfRow',
                                        'num value size type '
                                        'bind vis ndx name')

    records = [line.strip(" ").split() for line in output.splitlines()[3:]]
    tuples = [ReadelfRow(*fields) for fields in records if len(fields) == 8]
    return [fields.name for fields in tuples
            if (fields.type == 'OBJECT') and is_variable(fields.name)]


def list_of_locals():
    current_frame = gdb.selected_frame()
    current_block = current_frame.block()

    result = [str(symbol) for symbol in current_block
              if symbol.is_variable or symbol.is_argument]

    while current_block.function is None:
        current_block = current_block.superblock
        result.extend([str(symbol) for symbol in current_block
                       if symbol.is_variable or symbol.is_argument])

    return result


def get_typename_of(var):
    object = gdb.parse_and_eval(var)
    return str(object.type)


def has_type(var, typename):
    typename_of_var = get_typename_of(var)

    type_decl = _remove_spaces(typename_of_var)
    return (_remove_spaces(typename) == type_decl)


def _remove_spaces(typename):
    return typename.replace(' ', '')
