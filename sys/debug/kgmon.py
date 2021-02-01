import gdb
import sys


from ctypes import *
from .cmd import SimpleCommand
from .struct import GdbStructMeta

PROGRES_BAR_SIZE = 20
OFILE = "gmon.out"

def nextBar(nomin, denom, bar):
    return int(nomin * bar / denom) < int((nomin + 1) * bar / denom)


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
        sizeofgmonhdr = 32
        GMONVERSION = 333945

        gmonhdr = GmonHeader()
        gmonhdr.lpc = self.lowpc
        gmonhdr.hpc = self.highpc
        gmonhdr.ncnt = c_int(self.kcountsize + sizeofgmonhdr)
        gmonhdr.version = GMONVERSION
        gmonhdr.spare[0] = 0
        gmonhdr.spare[1] = 0
        gmonhdr.spare[2] = 0
        gmonhdr.profrate = 1000
        with open(OFILE, "wb") as of:
            of.write(gmonhdr)

    def writetickbuffer(self):
        kcountsize = self.kcountsize
        for i in range(kcountsize):
            if nextBar(i, kcountsize, PROGRES_BAR_SIZE):
                print("X", end='')
                sys.stdout.flush()
            kcount = gdb.parse_and_eval('_gmonparam.kcount[' + str(i) + ']')
            with open(OFILE, "ab") as of:
                of.write(c_ushort(kcount))

    def writearcinfo(self):
        rawarc = RawArc()
        sizeoffroms_p = gdb.parse_and_eval('sizeof(*_gmonparam.froms)')
        endfrom = self.fromssize / sizeoffroms_p

        for fromindex in range(endfrom):
            if nextBar(fromindex, endfrom, PROGRES_BAR_SIZE):
                print("X", end='')
                sys.stdout.flush()
            froms = gdb.parse_and_eval(f'_gmonparam.froms[{fromindex}]')
            if froms == 0:
                continue
            frompc = self.lowpc
            frompc += fromindex * self.hashfraction * sizeoffroms_p
            toindex = froms

            while toindex != 0:
                prefix = f'_gmonparam.tos[{toindex}].'
                rawarc.raw_frompc = frompc
                rawarc.raw_selfpc = gdb.parse_and_eval(prefix + 'selfpc')
                rawarc.raw_count = gdb.parse_and_eval(prefix + 'count')
                with open(OFILE, "ab") as of:
                    of.write(rawarc)
                toindex = gdb.parse_and_eval(prefix + 'link')


class Kgmon(SimpleCommand):
    """Dump the data to gmon.out"""

    def __init__(self):
        super().__init__('kgmon')

    def __call__(self, args):
        gmonparam = GmonParam(gdb.parse_and_eval('_gmonparam'))
        gmonparam.writeheader()
        print("KGMON: Tick buffer")
        gmonparam.writetickbuffer()
        print("\nKGMON: Arc info")
        gmonparam.writearcinfo()
        print("\nKGMON: Finished")
