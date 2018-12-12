#!/usr/bin/env python3
"""
Convert DTS (Device Tree Source) into a C array.
"""
import sys
import argparse
from string import Template
from functools import reduce

import fdt


DEVICE_HINTS_TEMPLATE = Template("""
/* Device hints internal representation.
 *
 * WARNING: This file is autogenerated and should NOT be modified directly.
 * Please refer to `device_hints.dts` file in case you would like to change
 * any of the values.
 */
#include <stdint.h>

typedef struct {
    const char *path;
    uint32_t *iomem;
    uint32_t *ioport;
    uint32_t irq;
} devhint_t;

devhint_t hints[] = {
${hints}
};
""")


def generate_fdt(filename):
    with open(filename, 'r') as f:
        data = f.read()
        dtb = fdt.parse_dts(data)
    return dtb


def generate_hints(device, path):
    for prop in device.props:
        yield ('.path', path)

        if prop.name == 'iomem':
            yield ('.iomem', prop.data)

        elif prop.name == 'ioport':
            yield ('.ioport', prop.data)

        elif prop.name == 'interrupts':
            assert len(prop.data) == 1, "Only one irq per device supported!"
            yield ('.irq', prop.data[0])

        else:
            raise ValueError("The following device hint resource is "
                             "not supported: {}".format(prop.name))


def flatten_fdt(root, path):
    child_hints = [flatten_fdt(node, "{}/{}".format(path, node.name))
                   for node in root.nodes]

    hints = reduce(lambda a, b: {**a, **b}, child_hints, {})
    current_device_hints = dict(generate_hints(root, path))
    if current_device_hints:
        hints[path] = current_device_hints
    return hints


def hint_as_c_entry(hint):
    to_c_value = (lambda val: {
        str: lambda: '"{}"'.format(val),
        int: lambda: hex(val),
        list: lambda: '(uint32_t[]){{{}, 0}}'.format(", ".join(map(str, val))),
    }[type(val)]())

    fields_as_strs = [
        "\t{} = {}".format(resource, to_c_value(value))
        for resource, value in hint.items()
    ]
    entry = ',\n'.join(fields_as_strs)
    return "{{ {}\n}},\n".format(entry)


def device_hints_as_c_array(hints):
    return ''.join(map(hint_as_c_entry, hints.values()))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
            description='Converting DTS (Device Tree Source) into a C array.')
    parser.add_argument('input', metavar='DTS', type=str,
                        help='Device Tree Source file')
    parser.add_argument('output', metavar='SRC', type=str,
                        help='C array of devhint_t structures')
    args = parser.parse_args()

    fdt = generate_fdt(args.input)
    flat_hints = flatten_fdt(fdt.root_node, '/rootdev')
    hints_as_c_array = device_hints_as_c_array(flat_hints)
    with open(args.output, 'w') as f:
        f.write(DEVICE_HINTS_TEMPLATE.substitute(hints=hints_as_c_array))
