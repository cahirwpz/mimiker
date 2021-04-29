import gdb

from .cmd import SimpleCommand
from struct import *


def gmon_write(path):
    infer = gdb.inferiors()[0]

    with open(path, "wb") as of:
        # Write headers
        gmonhdr_size = int(gdb.parse_and_eval('sizeof(_gmonhdr)'))
        gmonhdr_p = gdb.parse_and_eval('&_gmonhdr')
        of.write(infer.read_memory(gmonhdr_p, gmonhdr_size))

        # Write tick buffer
        kcountsize = int(gdb.parse_and_eval('_gmonparam.kcountsize'))
        kcount = gdb.parse_and_eval('_gmonparam.kcount')
        of.write(infer.read_memory(kcount, kcountsize))

        # Write arc info
        froms_el_size = int(gdb.parse_and_eval('sizeof(*_gmonparam.froms)'))
        fromssize = int(gdb.parse_and_eval('_gmonparam.fromssize'))
        tossize = int(gdb.parse_and_eval('_gmonparam.tossize'))
        hashfraction = int(gdb.parse_and_eval('_gmonparam.hashfraction'))
        froms = gdb.parse_and_eval('_gmonparam.froms')
        tos = gdb.parse_and_eval('_gmonparam.tos')
        lowpc = int(gdb.parse_and_eval('_gmonparam.lowpc'))

        memory = infer.read_memory(froms, fromssize)
        froms_array = unpack('H' * int(fromssize/calcsize('H')), memory)
        memory = infer.read_memory(tos, tossize)
        tos_array = unpack('IiHH' * int(tossize/calcsize('IiHH')), memory)

        fromindex = 0
        for from_val in froms_array:
            # Nothing has been called from this function
            if from_val == 0:
                continue
            # Getting the calling function addres from encoded value
            frompc = lowpc + fromindex * froms_el_size * hashfraction
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
            print("Compile program with KGPROF=1 or gmon not initialized yet")
        elif state == gdb.parse_and_eval('GMON_PROF_BUSY'):
            # To ensure consistent data
            print("The mcount function is running - wait for it to finish")
        else:
            if state == gdb.parse_and_eval('GMON_PROF_ERROR'):
                print("The tostruct array was too small for the whole process")
            gmon_write(args or 'gmon.out')
