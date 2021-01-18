import gdb


from ctypes import *
from .cmd import SimpleCommand
from .struct import GdbStructMeta

class RawArc(Structure):
  _fields_ = [('raw_frompc', c_ulong), ('raw_selfpc', c_ulong), ('raw_count', c_long)]
class ToStruct(Structure):
  _fields_ = [('selfpc', c_ulong), ('count', c_long), ('link', c_ushort), ('pad', c_ushort)]

class GmonHeader(Structure):
  _fields_ = [('lpc', c_ulong), ('hpc', c_ulong), ('ncnt', c_int), ('version', c_int), ('profrate', c_int), ('spare', c_int * 3)]

class GmonParam(metaclass=GdbStructMeta):
  __ctype__ = 'struct gmonparam'
  __cast__ = {'state': int, 'kcount': POINTER(c_ushort), 'kcountsize': int,
              'froms': int, 'fromssize': int, 'tos': POINTER(ToStruct),
              'tossize': int, 'tolimit': int, 'lowpc': int, 'highpc': int,
              'textsize': int, 'hashfraction': int}

  def writeheader(self):
    gmonhdr = GmonHeader()
    gmonhdr.lpc = self.lowpc
    gmonhdr.hpc = self.highpc
    sizeofgmonhdr = 32
    gmonhdr.ncnt = c_int(self.kcountsize + sizeofgmonhdr)
    GMONVERSION =	0x00051879
    gmonhdr.version = GMONVERSION
    # We do not have timer for profiling yet
    gmonhdr.profrate = 0
    ofile = "gmon.out"
    with open(ofile, "wb") as of:
      of.write(gmonhdr)
  
  # Not supported yet
  def writetickbuffer(self):
    pass

  def writearcinfo(self):
    rawarc = RawArc()
    sizeoffroms_p = gdb.parse_and_eval('sizeof(*_gmonparam.froms)')
    endfrom = self.fromssize / sizeoffroms_p
    for fromindex in range(endfrom):
      print(f"from {fromindex} to {endfrom}")      
      froms = gdb.parse_and_eval(f'_gmonparam.froms[{fromindex}]')
      if froms == 0:
        continue
      frompc = self.lowpc
      frompc += fromindex * self.hashfraction * sizeoffroms_p
      toindex = froms
      while toindex != 0:
        print(f"toindex {toindex}")
        rawarc.raw_frompc = frompc
        to = gdb.parse_and_eval(f'_gmonparam.tos[{toindex}]')
        rawarc.raw_selfpc =  gdb.parse_and_eval(f'_gmonparam.tos[{toindex}].selfpc')
        rawarc.raw_count =  gdb.parse_and_eval(f'_gmonparam.tos[{toindex}].count')
        ofile = "gmon.out"
        with open(ofile, "ab") as of:
          of.write(rawarc)
        toindex =  gdb.parse_and_eval(f'_gmonparam.tos[{toindex}].link')

class Kgmon(SimpleCommand):
  """Dump the data to kgmon.out"""
  
  def __init__(self):
    super().__init__('kgmon')

  def __call__(self, args):
    gmonparam = GmonParam(gdb.parse_and_eval('_gmonparam'))
    gmonparam.writeheader()
    gmonparam.writetickbuffer()
    gmonparam.writearcinfo()

    print('Wywołano kgmon :)')