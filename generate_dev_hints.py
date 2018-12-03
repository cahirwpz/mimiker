import sys

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
    props = [
        node for node in device.subdata
        if isinstance(node, pyfdt.FdtProperty)
    ]
    for prop in props:
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
    child_hints = [
        flatten_ftd(node, "{}/{}".format(path, node.name))
        for node in root.subdata
        if isinstance(node, pyfdt.FdtNode)
    ]

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

    fields_as_strs = [
        "\t{} = {}".format(resource, to_c_value(value))
        for resource, value in hint.iteritems()
    ]
    entry = ',\n'.join(fields_as_strs)
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
