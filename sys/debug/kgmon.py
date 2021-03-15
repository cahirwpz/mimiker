import gdb
import sys


from ctypes import *
from struct import unpack
from .cmd import SimpleCommand
from .struct import GdbStructMeta

OFILE = "gmon.out"


class RawArc(Structure):
    _fields_ = [('raw_frompc', c_uint), ('raw_selfpc', c_uint),
                ('raw_count', c_int)]


class ToStruct(Structure):
    _fields_ = [('selfpc', c_uint), ('count', c_int),
                ('link', c_ushort), ('pad', c_ushort)]


class GmonParam(metaclass=GdbStructMeta):
    __ctype__ = 'struct gmonparam'
    __cast__ = {'state': int, 'kcount': POINTER(c_ushort), 'kcountsize': int,
                'froms': int, 'fromssize': int, 'tos': POINTER(ToStruct),
                'tossize': int, 'tolimit': int, 'lowpc': int, 'highpc': int,
                'textsize': int, 'hashfraction': int}

    def writeheader(self, inferior):
        gmonhdr_size = int(gdb.parse_and_eval('sizeof(_gmonhdr)'))
        gmonhdr_p = gdb.parse_and_eval('&_gmonhdr')
        memory = inferior.read_memory(gmonhdr_p, gmonhdr_size)
        array = unpack('IIiiiiii', memory)
        with open(OFILE, "wb") as of:
            for i in array:
                of.write(c_int(i))

    def writetickbuffer(self, inferior):
        kcountsize = self.kcountsize
        kcount = gdb.parse_and_eval('_gmonparam.kcount')
        memory = inferior.read_memory(kcount, kcountsize * sizeof(c_ushort))
        array = unpack(kcountsize * 'H', memory)
        with open(OFILE, "ab") as of:
            for i in array:
                of.write(c_ushort(i))

    def writearcinfo(self, inferior):
        rawarc = RawArc()
        froms_p_size = gdb.parse_and_eval('sizeof(*_gmonparam.froms)')
        endfrom = int(self.fromssize / froms_p_size)

        froms = gdb.parse_and_eval('_gmonparam.froms')
        memory = inferior.read_memory(froms, endfrom * sizeof(c_ushort))
        array = unpack(endfrom * 'H', memory)

        for i in array:
            if i == 0:
                continue
            frompc = self.lowpc
            frompc += fromindex * self.hashfraction * froms_p_size
            toindex = froms

            while toindex != 0:
                tos = gdb.parse_and_eval('_gmonparam.tos')
                memory = inferior.read_memory(tos, sizeof(ToStruct))
                read = unpack('IiHH', memory)
                rawarc.raw_frompc = frompc
                rawarc.raw_selfpc = read[0]
                rawarc.raw_count = read[1]
                with open(OFILE, "ab") as of:
                    of.write(rawarc)
                toindex = read[2]


class Kgmon(SimpleCommand):
    """Dump the gprof data to gmon.out"""

    def __init__(self):
        super().__init__('kgmon')

    def __call__(self, args):
        state = gdb.parse_and_eval('_gmonparam.state')
        if state == gdb.parse_and_eval('GMON_PROF_NOT_INIT'):
            print("Compile program with KPROF=1 or gmon not innitalized yet")
            return
        infer = gdb.inferiors()[0]
        gmonparam = GmonParam(gdb.parse_and_eval('_gmonparam'))
        gmonparam.writeheader(infer)
        gmonparam.writetickbuffer(infer)
        gmonparam.writearcinfo(infer)
        print("KGMON: Finished")
