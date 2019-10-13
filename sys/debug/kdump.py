from .physmem import KernelSegments, KernelFreePages
from .virtmem import VMMapSegments
from .cmd import CommandDispatcher


class Kdump(CommandDispatcher):
    """Examine kernel data structures."""

    def __init__(self):
        super().__init__('kdump', [KernelSegments(), KernelFreePages(),
                                   VMMapSegments()])
