import gdb
import sys


from .cmd import SimpleCommand


class GmonOut():

    @staticmethod
    def write_to_file(path="gmon.out"):
        infer = gdb.inferiors()[0]
        with open(path, "wb") as of:
            GmonOut.write_header(infer, of)
            GmonOut.write_tick_buffer(infer, of)
            GmonOut.write_arc_info(infer, of)

    @staticmethod
    def write_header(inferior, file):
        gmonhdr_size = int(gdb.parse_and_eval('sizeof(_gmonhdr)'))
        gmonhdr_p = gdb.parse_and_eval('&_gmonhdr')
        file.write(inferior.read_memory(gmonhdr_p, gmonhdr_size))

    @staticmethod
    def write_tick_buffer(inferior, file):
        kcountsize = int(gdb.parse_and_eval('_gmonparam.kcountsize'))
        kcount = gdb.parse_and_eval('_gmonparam.kcount')
        file.write(inferior.read_memory(kcount, kcountsize))

    @staticmethod
    def write_arc_info(inferior, file):
        return


class Kgmon(SimpleCommand):
    """Dump the gprof data to gmon.out"""

    def __init__(self):
        super().__init__('kgmon')

    def __call__(self, args):
        state = gdb.parse_and_eval('_gmonparam.state')
        if state == gdb.parse_and_eval('GMON_PROF_NOT_INIT'):
            print("Compile program with KGPROF=1 or gmon not initialized yet")
            return
        gmon = GmonOut.write_to_file()
        print("KGMON - finished successfully")
