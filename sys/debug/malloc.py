import gdb

from .cmd import UserCommand
from .struct import TailQueue
from .utils import global_var


class Malloc(UserCommand):
    """List boundary tags in all arenas."""

    def __init__(self):
        super().__init__('malloc')

        self.bins = [(0, 31)]
        for s in list(range(32, 128, 16)):
            self.bins.append((s, s + 16 - 1))
        for i in range(7, 16):
            for s in range(2**i, 2**(i+1), 2**(i-2)):
                self.bins.append((s, s + 2**(i-2) - 1))
        self.bins.append((2**16, 2**18 - 1))

    def __call__(self, args):
        word = gdb.lookup_type('word_t')
        word_ptr = word.pointer()
        word_size = word.sizeof
        canary = 0xDEADC0DE

        arena_list = TailQueue(global_var('arena_list'), 'link')
        dangling = 0

        for arena in arena_list:
            start = arena['start'].cast(word)
            end = arena['end'].cast(word)

            print("[arena] start: 0x%X, end: 0x%X" % (start, end))

            # Check boundary tag layout.
            ptr = start
            prevfree = False
            is_last = False

            while ptr < end:
                btag = ptr.cast(word_ptr).dereference()
                is_used = bool(btag & 1)
                is_prevfree = bool(btag & 2)
                is_last = bool(btag & 4)
                size = btag & -8
                # Ok... now let's check validity
                footer_ptr = ptr + size - word_size
                footer = footer_ptr.cast(word_ptr).dereference()
                if is_used:
                    is_valid = (is_prevfree == prevfree) and (footer == canary)
                    prevfree = False
                else:
                    is_valid = (btag == footer) or (not prevfree)
                    prevfree = True
                    dangling += 1
                # Print the block and proceed
                print("  0x%X: [%c%c:%u] %c %s" % (
                    ptr, "FU"[int(is_used)], " P"[int(is_prevfree)], size,
                    " *"[int(is_last)], ["(invalid!)", ""][int(is_valid)]))
                ptr += size

            if not is_last:
                print("(***) Last block set incorrectly!")

        # Check buckets of free blocks.
        freelst = global_var('freebins')
        idx_from, idx_to = freelst.type.range()
        for i in range(idx_from, idx_to + 1):
            head = freelst[i].address
            node = head['next']
            if node == head:
                continue
            print("[free:%d-%d] first: 0x%X, last: 0x%X" % (
                self.bins[i][0], self.bins[i][1], head['next'].cast(word),
                head['prev'].cast(word)))
            while node != head:
                ptr = node.cast(word) - word_size
                btag = ptr.cast(word_ptr).dereference()
                # Ok... now let's check validity
                is_used = bool(btag & 1)
                is_valid = not is_used
                dangling -= 1
                # Print the block and proceed
                print("  0x%X: [0x%X, 0x%X] %s" % (
                    node.cast(word), node['prev'].cast(word),
                    node['next'].cast(word),
                    ["(invalid!)", ""][int(is_valid)]))
                node = node['next']

        if dangling != 0:
            print("(***) Some free blocks are not inserted on free list!")
