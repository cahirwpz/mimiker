import gdb

from .cmd import SimpleCommand
from .struct import GdbStructMeta
from struct import *


class GmonParam(metaclass=GdbStructMeta):
    __ctype__ = 'struct gmonparam'
    __cast__ = {'state': int, 'kcount': int, 'kcountsize': int,
                'froms': int, 'fromssize': int, 'tos': int,
                'tossize': int, 'tolimit': int, 'lowpc': int, 'highpc': int,
                'textsize': int, 'hashfraction': int}


def gmon_write(path):
    infer = gdb.inferiors()[0]
    gparam = GmonParam(gdb.parse_and_eval('_gmonparam'))

    with open(path, "wb") as of:
        # Write headers
        gmonhdr_size = int(gdb.parse_and_eval('sizeof(_gmonhdr)'))
        gmonhdr_p = gdb.parse_and_eval('&_gmonhdr')
        of.write(infer.read_memory(gmonhdr_p, gmonhdr_size))

        # Write tick buffer
        kcount = gdb.parse_and_eval('_gmonparam.kcount')
        of.write(infer.read_memory(kcount, gparam.kcountsize))

        # Write arc info
        froms_el_size = int(gdb.parse_and_eval('sizeof(*_gmonparam.froms)'))
        froms_p = gdb.parse_and_eval('_gmonparam.froms')
        tos_p = gdb.parse_and_eval('_gmonparam.tos')

        memory = infer.read_memory(froms_p, gparam.fromssize)
        froms_array = unpack('H' * int(gparam.fromssize/calcsize('H')), memory)
        memory = infer.read_memory(tos_p, gparam.tossize)
        size = calcsize('IiHH')
        tos_array = unpack('IiHH' * int(gparam.tossize/size), memory)

        fromindex = 0
        for from_val in froms_array:
            # Nothing has been called from this function
            if from_val == 0:
                continue
            # Getting the calling function addres from encoded value
            offset = fromindex * froms_el_size * gparam.hashfraction
            frompc = gparam.lowpc + offset
            toindex = from_val

            # Traversing the tos list for the calling function
            # It stores data about called functions
            while toindex != 0:
                selfpc = tos_array[toindex * 4]
                count = tos_array[toindex * 4 + 1]
                toindex = tos_array[toindex * 4 + 2]
                of.write(pack('IIi', frompc, selfpc, count))
            fromindex += 1


class Kgmon(SimpleCommand):
    """Dump the gprof data to file (by default 'gmon.out')"""

    def __init__(self):
        super().__init__('kgmon')

    def __call__(self, args):
        args = args.strip()
        state = gdb.parse_and_eval('_gmonparam.state')
        if state == gdb.parse_and_eval('GMON_PROF_NOT_INIT'):
            print("Kgprof not initialized yet")
        elif state == gdb.parse_and_eval('GMON_PROF_BUSY'):
            # To ensure consistent data
            print("The mcount function is running - wait for it to finish")
        else:
            if state == gdb.parse_and_eval('GMON_PROF_ERROR'):
                print("The tostruct array was too small for the whole process")
            gmon_write(args or 'gmon.out')
