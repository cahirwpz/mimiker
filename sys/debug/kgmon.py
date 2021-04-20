import gdb

from .cmd import SimpleCommand
from struct import *


def gmon_write(path):
    infer = gdb.inferiors()[0]
    long_size = int(gdb.parse_and_eval('sizeof(long)'))
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
        froms_p_size = int(gdb.parse_and_eval('sizeof(*_gmonparam.froms)'))
        fromssize = int(gdb.parse_and_eval('_gmonparam.fromssize'))
        tossize = int(gdb.parse_and_eval('_gmonparam.tossize'))
        hashfraction = gdb.parse_and_eval('_gmonparam.hashfraction')
        froms = gdb.parse_and_eval('_gmonparam.froms')
        tos = gdb.parse_and_eval('_gmonparam.tos')
        lowpc = gdb.parse_and_eval('_gmonparam.lowpc')

        memory = infer.read_memory(froms, fromssize)
        froms_array = unpack('H' * int(fromssize/calcsize('H')), memory)
        memory = infer.read_memory(tos, tossize)

        if long_size == 32:
            tos_array = unpack('IiHH' * int(tossize/calcsize('IiHH')), memory)
        elif long_size == 64:
            tos_array = unpack('LlHH' * int(tossize/calcsize('IiHH')), memory)

        index = 0
        for from_val in froms_array:
            if from_val == 0:
                continue
            frompc = lowpc + index * froms_p_size
            toindex = from_val

            while toindex != 0:
                selfpc = tos_array[toindex * 4]
                count = tos_array[toindex * 4 + 1]
                toindex = tos_array[toindex * 4 + 2]
                if long_size == 32:
                    of.write(pack('IIi', frompc, selfpc, count))
                elif long_size == 64:
                    of.write(pack('LLl', frompc, selfpc, count))
            index += 1


class Kgmon(SimpleCommand):
    """Dump the gprof data to file (by default 'gmon.out')"""

    def __init__(self):
        super().__init__('kgmon')

    def __call__(self, args):
        args = args.strip()
        state = gdb.parse_and_eval('_gmonparam.state')
        if state == gdb.parse_and_eval('GMON_PROF_NOT_INIT'):
            print("Compile program with KGPROF=1 or gmon not initialized yet")
        else:
            gmon_write(args or 'gmon.out')
