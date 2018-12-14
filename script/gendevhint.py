#!/usr/bin/env python3

import os
import argparse
from string import Template

import fdt


OUTPUT_TEMPLATE = Template("""
/* Device hints internal representation.
 *
 * WARNING: This file is autogenerated and should NOT be modified directly.
 * Please refer to `${input}` file in case you would like to change
 * any of the values.
 */
#include <stdint.h>

/* TODO: Move structures to device.h ? */

/* memory or io-port address range */
typedef struct dh_range {
  uintptr_t start, end;
} dh_range_t;

typedef struct {
  const char *path;
  dh_range_t **iomem;
  dh_range_t **ioport;
  uint32_t irq;
} devhint_t;

devhint_t ${platform}_hints[] = {
${hints}
};
""")


def generate_fdt(filename):
    with open(filename, 'r') as f:
        data = f.read()
        dtb = fdt.parse_dts(data)
    return dtb


def flatten_fdt(root, path):
    nodes = [(path, root)]
    for node in root.nodes:
        nodes.extend(flatten_fdt(node, path + '/' + node.name))
    return nodes


def make_pairs(lst):
    return [(lst[i], lst[i + 1]) for i in range(0, len(lst), 2)]


def convert_node(path, device):
    hints = {}
    for prop in device.props:
        hints['.path'] = path
        if prop.name == 'iomem':
            hints['.iomem'] = make_pairs(prop.data)
        elif prop.name == 'ioport':
            hints['.ioport'] = make_pairs(prop.data)
        elif prop.name == 'interrupts':
            assert len(prop.data) == 1, "Only one irq per device supported!"
            hints['.irq'] = prop.data[0]
        else:
            raise ValueError("The following device hint resource is "
                             "not supported: {}".format(prop.name))
    return hints


def convert_to_hints(nodes):
    return filter(bool, [convert_node(*node) for node in nodes])


def to_c_value(val):
    if isinstance(val, str):
        return '"' + val + '"'
    if isinstance(val, int):
        return hex(val)
    if isinstance(val, list):
        ranges = ['&(dh_range_t){.start = 0x%x, .end = 0x%x}' % pair
                  for pair in val]
        ranges.append('(dh_range_t *)0')
        return '(dh_range_t *[]){\n    ' + ',\n    '.join(ranges) + '\n  }'
    raise ValueError(repr(val))


def hint_as_c_entry(hint):
    fields = ['  {} = {}'.format(resource, to_c_value(value))
              for resource, value in hint.items()]
    return '{\n' + ',\n'.join(fields) + '\n}'


def hints_as_c_array(hints):
    return ',\n'.join(map(hint_as_c_entry, hints))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
            description='Convert DTS (Device Tree Structure) into a C array.')
    parser.add_argument('input', metavar='DTS', type=str,
                        help='Device Tree Structure file')
    parser.add_argument('output', metavar='SRC', type=str,
                        help='C array of devhint_t structures')
    args = parser.parse_args()

    fdtree = generate_fdt(args.input)
    node_list = flatten_fdt(fdtree.root_node, '/rootdev')
    flat_hints = convert_to_hints(node_list)
    hints_array = hints_as_c_array(flat_hints)
    with open(args.output, 'w') as f:
        f.write(OUTPUT_TEMPLATE.substitute(
            input=args.input,
            platform=os.path.splitext(args.input)[0].replace('/', '_'),
            hints=hints_array))
