import gdb
import sys


from ctypes import *
from struct import unpack
from .cmd import SimpleCommand
from .struct import GdbStructMeta

class GmonOut():

    def write_to_file(self, path="gmon.out"):
        infer = gdb.inferiors()[0]
        with open(path, "wb") as of:
            self.write_header(infer, of)
            self.write_tick_buffer(infer, of)
            self.write_arc_info(infer, of)

    def write_header(self, inferior, file):
        gmonhdr_size = int(gdb.parse_and_eval('sizeof(_gmonhdr)'))
        gmonhdr_p = gdb.parse_and_eval('&_gmonhdr')
        memory = inferior.read_memory(gmonhdr_p, gmonhdr_size)
        array = unpack('IIiiiiii', memory)
        for i in array:
            file.write(c_int(i))

    def write_tick_buffer(self, inferior, file):
        kcountsize = int(gdb.parse_and_eval('_gmonparam.kcountsize'))
        kcount = gdb.parse_and_eval('_gmonparam.kcount')
        memory = inferior.read_memory(kcount, kcountsize * sizeof(c_ushort))
        array = unpack(kcountsize * 'H', memory)
        for i in array:
            file.write(c_ushort(i))

    def write_arc_info(self, inferior, file):
        return


class Kgmon(SimpleCommand):
    """Dump the gprof data to gmon.out"""

    def __init__(self):
        super().__init__('kgmon')

    def __call__(self, args):
        state = gdb.parse_and_eval('_gmonparam.state')
        if state == gdb.parse_and_eval('GMON_PROF_NOT_INIT'):
            print("Compile program with KPROF=1 or gmon not innitalized yet")
            return
        gmon = GmonOut().write_to_file()
        print("KGMON: Finished")
