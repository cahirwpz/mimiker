from __future__ import annotations

from enum import Enum
from typing import Dict, List, Tuple


class KFTEventType(Enum):
    KFT_IN = 0
    KFT_OUT = 1


class KFTEvent():
    """
    Internal representation of KFT event
    """

    def __init__(self, typ: int, pc: int, timestamp: int):
        self.typ = KFTEventType(typ)
        self.pc = pc
        self.timestamp = timestamp

    def __repr__(self) -> str:
        type = 'in' if self.typ == KFTEventType.KFT_IN else 'out'
        return f'<{type} {self.pc:#x} {self.timestamp}>'

    @staticmethod
    def decode(v: int, kern_start: int) -> Tuple[int, KFTEvent]:
        """
        Decode kft event from compressed representation of the event.

        Arguments:
            v - encoded value
            kern_start - kernel start address

        Returns:
            thread id
            KFTEvent class
        """
        PC_MASK = 0xFFFFFC            # 24 bits
        TIMESTAMP_MASK = 0x1FFFFFFFF  # 31 bits
        THREAD_MASK = 0xFF            # 8 bits
        TYPE_MASK = 0x1               # 1 bit

        PC_SHIFT = 40
        TIMESTAMP_SHIFT = 9
        THREAD_SHIFT = 1
        TYPE_SHIFT = 0

        typ = (v >> TYPE_SHIFT) & TYPE_MASK
        thread = (v >> THREAD_SHIFT) & THREAD_MASK
        timestamp = (v >> TIMESTAMP_SHIFT) & TIMESTAMP_MASK
        rel_pc = (v >> PC_SHIFT) & PC_MASK
        pc = kern_start + rel_pc

        return thread, KFTEvent(typ, pc, timestamp)


TdEvents = Dict[int, List[KFTEvent]]
