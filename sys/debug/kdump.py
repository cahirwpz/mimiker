from .virtmem import VmPhysSeg, VmFreePages, VmMapSeg, PhysMap
from .memory import Vmem, MallocStats, PoolStats
from .cmd import CommandDispatcher


class Kdump(CommandDispatcher):
    """Examine kernel data structures."""

    def __init__(self):
        super().__init__('kdump', [VmPhysSeg(), VmFreePages(), VmMapSeg(),
                                   PhysMap(), Vmem(), MallocStats(),
                                   PoolStats()])
