import sys
import json

from pyfdt import pyfdt


def generate_fdt(filename):
    with open(filename) as f:
        dtb = pyfdt.FdtBlobParse(f)
    return dtb.to_fdt()


def flatten_ftd(root, path):
    is_node = lambda node: isinstance(node, pyfdt.FdtNode)
    is_property = lambda node: isinstance(node, pyfdt.FdtProperty)

    get_node_path = lambda node: f"{path}/{node.name}"
    child_hints = map(lambda node: flatten_ftd(node, get_node_path(node)),
                      filter(is_node, root.subdata))

    merged_hints = reduce(lambda acc, res: {**acc, **res}, child_hints, {})
    merged_hints[path] =  {
        p.name: p.words
        for p in filter(is_property, root.subdata)
    }
    return results


def help():
    # TODO: format output according to standard
    print("Pass Device Tree Blop (dtb) file to generate a static C array")


def main(*args):
    if len(sys.argv) != 1:
        help()

    filename = sys.argv[1]
    fdt = generate_fdt(filename)
    flat_hints = flatten_ftd(fdt.get_rootnode())


if __name__ == "__main__":
    main()

