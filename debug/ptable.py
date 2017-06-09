from __future__ import print_function
import ctypes


def as_uint32(num):
    return ctypes.c_ulong(num).value & 0xffffffff


def as_hex(num):
    return "$%08x" % as_uint32(num)


def cellfmt(cell, fmt, width):
    if fmt == 'l':
        return cell.ljust(width)
    if fmt == 'c':
        return cell.center(width)
    return cell.rjust(width)


def ptable(rows, fmt=None, header=False):
    """
    rows : a list of items (all items will be stringified)
    fmt : string of chars 'lrc' for text justification (default: right)
    header : first row will be a table header if True
    """
    if not rows:
        return

    fmt = fmt or []
    columns = max(map(len, rows))
    rows = [row + ([''] * (columns - len(row))) for row in rows]
    width = [max([len(row[c]) for row in rows]) for c in range(columns)]
    if len(fmt) < columns:
        fmt += 'r' * (columns - len(fmt))

    hline = '-' * (sum(width) + len(width) * 3 + 1)

    print(hline)
    for i, row in enumerate(rows):
        cells = [cellfmt(str(row[c]), fmt[c], width[c])
                 for c in range(columns)]
        print('| %s |' % ' | '.join(cells))
        if i == 0 and header:
            print(hline)

    if not header or (header and len(rows) > 1):
        print(hline)
