def format_cell(cell, fmt, column_size):
    spaces = column_size - len(cell)
    if fmt == 'l':
        cell = cell + ' ' * spaces
    elif fmt == 'c':
        cell = ' ' * (spaces / 2) + cell + ' ' * ((spaces + 1) / 2)
    else:
        cell = ' ' * spaces + cell
    return cell

def ptable(rows, fmt=None, header=False):
    """
    rows : a list of items (all items will be stringified)
    fmt : string of characters 'lrc' for text justification (all right justified if None)
    header : first row will be a table header if True
    """
    if len(rows) == 0:
        return
    
    if fmt == None:
        fmt = []
    columns_cnt = max(len(row) for row in rows)
    rows = [row + ([''] * (columns_cnt - len(row))) for row in rows]
    columns_size = [max([len(row[c]) for row in rows]) for c in range(columns_cnt)]
    if len(fmt) < columns_cnt:
        fmt += 'r' * (columns_cnt - len(fmt))
    rows = [[format_cell(row[col], fmt[col], columns_size[col]) for col in range(len(row))] for row in rows]
    rows = ['| ' + ' | '.join(row) + ' |'for row in rows]
    
    line = '-' * len(rows[0])
    
    print line
    for i, row in enumerate(rows):
        print row
        if i==0 and header:
            print line
    
    if not header or header and len(rows)>1:
        print line