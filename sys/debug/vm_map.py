from .struct import GdbStructMeta
from .struct import TailQueue


class VmMap(metaclass=GdbStructMeta):
    __ctype__ = 'struct vm_map'

    def __repr__(self):
        return 'vm_map[entries=[{} {}], pmap={}]'.format(
            self.entries, self.nentries, self.pmap)

    def get_entries(self):
        entries = TailQueue(self.entries, 'link')
        return [VmMapEntry(e) for e in entries]


class VmMapEntry(metaclass=GdbStructMeta):
    __ctype__ = 'struct vm_map_entry'
    __cast__ = {'start': int,
                'end': int}

    def __repr__(self):
        return 'vm_map_entry[{:#08x}-{:#08x}]'.format(self.start, self.end)

    @property
    def amap_ptr(self):
        return self.aref['amap']

    @property
    def pages(self):
        size = self.end - self.start
        return int(size / 4096)

    @property
    def amap(self):
        if int(self.aref['amap']) == 0:
            return None
        else:
            return Amap(self.aref['amap'])

    @property
    def amap_offset(self):
        return self.aref['offset']

    def amap_bitmap_str(self):
        return self.amap.str_bitmap(self.amap_offset, self.pages)


class Amap(metaclass=GdbStructMeta):
    __ctype__ = 'struct vm_amap'
    __cast__ = {'slots': int,
                'ref_cnt': int}

    def __repr__(self):
        return 'vm_amap[slots={}, refs={}]'.format(self.slots, self.ref_cnt)

    def _get_bitmap(self):
        bitmap = []
        bitssize = ((self.slots) + 7) >> 3
        for i in range(bitssize):
            val = self.pg_bitmap[i]
            for _ in range(8):
                bitmap.append(val & 1 == 1)
                val >>= 1
        return bitmap

    def print_pages(self, offset, n):
        bm = self._get_bitmap()
        slots = 'Page num:'
        values = 'Ref cnt: '
        used = 0
        for i in range(n):
            slots += f'{i:^5}'
            if bm[offset + i]:
                values += f'{1:^5}'
                used += 1
            else:
                values += f'{"-": ^5}'

        print(f'Pages used by segment ({used} pages)')
        print(slots)
        print(values)
