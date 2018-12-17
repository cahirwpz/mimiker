from __future__ import print_function

import gdb
from tailq import TailQueue
import utils
import ptable
import traceback
from ctx import Context


class Process(object):
    __metaclass__ = utils.GdbStructMeta
    __ctype__ = 'struct proc'

    @staticmethod
    def pointer_type():
        return gdb.lookup_type('struct proc')

    @classmethod
    def current(cls):
        return cls(gdb.parse_and_eval('_pcpu_data->curthread->td_proc')
                   .dereference())

    @classmethod
    def from_pointer(cls, ptr):
        return cls(gdb.parse_and_eval('(struct proc *)' + ptr).dereference())

    @staticmethod
    def dump_list(processes):
        rows = [['Pid', 'Tid', 'State']]
        rows.extend([[str(p.p_pid), str(p.p_thread), str(p.p_state)]
                    for p in processes])
        ptable.ptable(rows, fmt='rll', header=True)

    @classmethod
    def list_all(cls):
        alive = TailQueue(gdb.parse_and_eval('proc_list'), 'p_all')
        dead = TailQueue(gdb.parse_and_eval('zombie_list'), 'p_all')
        return map(Process, list(alive) + list(dead))

    def __str__(self):
        return 'proc{pid=%d}' % self.p_pid


class Kprocess(gdb.Command, utils.OneArgAutoCompleteMixin):
    """ dump info about processes """

    def __init__(self):
        super(Kprocess, self).__init__('kproc', gdb.COMMAND_USER)

    def dump_all(self):
        Process.dump_list(Process.list_all())

    def invoke(self, args, from_tty):
        self.dump_all()

    def options(self):
        return None
