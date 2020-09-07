import gdb

from .cmd import UserCommand
from .struct import TailQueue
from .utils import global_var


class Malloc(UserCommand):
    """List boundary tags in all arenas."""

    def __init__(self):
        super().__init__('malloc')

    def __call__(self, args):
        word = gdb.lookup_type('word_t')
        word_ptr = word.pointer()

        arena_list = TailQueue(global_var('arena_list'), 'link')

        for arena in arena_list:
            ptr = arena['start'].cast(word)
            end = arena['end'].cast(word)

            while ptr < end:
                btag = ptr.cast(word_ptr).dereference()
                size = btag & -8
                print(ptr, btag)
                ptr += size
