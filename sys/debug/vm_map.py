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
