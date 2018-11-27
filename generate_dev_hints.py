import sys

from itertools import imap, ifilter
from functools import reduce

from pyfdt import pyfdt


def merge_dicts(a, b):
    a.update(b)
    return a


class UnsupportedHintError(Exception):
    pass


DEVICE_HINTS_TEMPLATE = """
#include <stdint.h>

typedef struct {{
    char* path;
    uint64_t iomem_start;
    uint64_t iomem_end;
    uint32_t irq;
}} devhint_t;

devhint_t hints[] = {{
{}
}};
"""


def generate_fdt(filename):
    with open(filename) as f:
        dtb = pyfdt.FdtBlobParse(f)
    return dtb.to_fdt()


def generate_hints(device, path):
    is_property = lambda node: isinstance(node, pyfdt.FdtProperty)
    for prop in ifilter(is_property, device.subdata):
        yield ('.path', path)

        if prop.name == 'reg':
            start = prop.words[0]
            end = prop.words[0] + prop.words[1]
            yield ('.iomem_start', start)
            yield ('.iomem_end', end)

        elif prop.name == 'interrupts':
            assert len(prop.words) == 1, "Only one irq per device supported!"
            yield ('.irq', prop.words[0])

        else:
            raise UnsupportedHintError("The following device hint resource is "
                                       "not supported: {}".format(prop.name))


def flatten_ftd(root, path):
    is_node = lambda node: isinstance(node, pyfdt.FdtNode)

    get_node_path = lambda node: "{}/{}".format(path, node.name)
    child_hints = imap(lambda node: flatten_ftd(node, get_node_path(node)),
                       ifilter(is_node, root.subdata))

    hints = reduce(merge_dicts, child_hints, {})
    current_device_hints = dict(generate_hints(root, path))
    if current_device_hints:
        hints[path] = current_device_hints
    return hints


def hint_as_c_entry(hint):
    to_c_value = (lambda val: {
        str: lambda: '"{}"'.format(val),
        int: lambda: hex(val),
    }[type(val)]())

    str_of_hint = lambda item: "\t{} = {}".format(item[0], to_c_value(item[1]))
    entry = ',\n'.join(map(str_of_hint, hint.iteritems()))
    return "{{ {} }},\n".format(entry)


def device_hints_as_c_array(hints):
    return ''.join(map(hint_as_c_entry, hints.itervalues()))


def help():
    # TODO: format output according to standard
    print("Pass Device Tree Blop (dtb) file to generate a static C array")
    exit(1)


def main(*args):
    if len(sys.argv) != 2:
        help()

    filename = sys.argv[1]
    fdt = generate_fdt(filename)
    flat_hints = flatten_ftd(fdt.get_rootnode(), '/rootdev')
    hints_as_c_array = device_hints_as_c_array(flat_hints)
    print(DEVICE_HINTS_TEMPLATE.format(hints_as_c_array))


if __name__ == "__main__":
    main()
