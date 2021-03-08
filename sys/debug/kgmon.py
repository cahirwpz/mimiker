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


class GmonHeader(Structure):
    _fields_ = [('lpc', c_uint), ('hpc', c_uint), ('ncnt', c_int),
                ('version', c_int), ('profrate', c_int), ('spare', c_int * 3)]


class GmonParam(metaclass=GdbStructMeta):
    __ctype__ = 'struct gmonparam'
    __cast__ = {'state': int, 'kcount': POINTER(c_ushort), 'kcountsize': int,
                'froms': int, 'fromssize': int, 'tos': POINTER(ToStruct),
                'tossize': int, 'tolimit': int, 'lowpc': int, 'highpc': int,
                'textsize': int, 'hashfraction': int}

    def writeheader(self):
        GMONVERSION = 333945
        gmonhdr = GmonHeader()
        gmonhdr.lpc = self.lowpc
        gmonhdr.hpc = self.highpc
        gmonhdr.ncnt = c_int(self.kcountsize + sizeof(GmonHeader))
        gmonhdr.version = GMONVERSION
        gmonhdr.spare[0] = gmonhdr.spare[1] = gmonhdr.spare[2] = 0
        # TO DO: read the profrate value
        gmonhdr.profrate = 1000
        with open(OFILE, "wb") as of:
            of.write(gmonhdr)

    def writetickbuffer(self, inferior):
        kcountsize = self.kcountsize
        kcount = gdb.parse_and_eval('_gmonparam.kcount[0]').address
        memory = inferior.read_memory(kcount, kcountsize * sizeof(c_ushort))
        array = unpack(kcountsize * 'H', memory)
        with open(OFILE, "ab") as of:
            for i in array:
                of.write(c_ushort(i))

    def writearcinfo(self, inferior):
        rawarc = RawArc()
        sizeoffroms_p = gdb.parse_and_eval('sizeof(*_gmonparam.froms)')
        endfrom = int(self.fromssize / sizeoffroms_p)

        froms = gdb.parse_and_eval(f'_gmonparam.froms[0]').address
        memory = inferior.read_memory(froms, endfrom * sizeof(c_ushort))
        array = unpack(endfrom * 'H', memory)

        for i in array:
            if i == 0:
                continue
            frompc = self.lowpc
            frompc += fromindex * self.hashfraction * sizeoffroms_p
            toindex = froms

            while toindex != 0:
                tos = gdb.parse_and_eval('_gmonparam.tos[0]').address
                memory = inferior.read_memory(tos, sizeof(ToStruct))
                read = unpack('IiHH', memory)
                rawarc.raw_frompc = frompc
                rawarc.raw_selfpc = read[0]
                rawarc.raw_count = read[1]
                with open(OFILE, "ab") as of:
                    of.write(rawarc)
                toindex = read[2]


class Kgmon(SimpleCommand):
    """Dump the data to gmon.out"""

    def __init__(self):
        super().__init__('kgmon')

    def __call__(self, args):
        infer = gdb.inferiors()[0]
        gmonparam = GmonParam(gdb.parse_and_eval('_gmonparam'))
        gmonparam.writeheader()
        gmonparam.writetickbuffer(infer)
        gmonparam.writearcinfo(infer)
        print("KGMON: Finished")
