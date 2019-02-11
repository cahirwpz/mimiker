from .thread import ThreadCreateBP, ThreadSwitchBP
from .cmd import CommandDispatcher, TraceCommand


class ThreadCreateTrace(TraceCommand):
    """Trace thread create events"""
    def __init__(self):
        super().__init__('thread_create', ThreadCreateBP)


class ThreadSwitchTrace(TraceCommand):
    """Trace thread switch events"""
    def __init__(self):
        super().__init__('thread_switch', ThreadSwitchBP)


class Ktrace(CommandDispatcher):
    """Trace important kernel events."""

    def __init__(self):
        super().__init__('ktrace', [ThreadCreateTrace(), ThreadSwitchTrace()])
