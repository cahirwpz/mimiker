import gdb

from .cmd import SimpleCommand


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

        # TODO: write arc info


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
