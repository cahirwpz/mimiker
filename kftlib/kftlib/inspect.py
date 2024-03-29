from __future__ import annotations

import logging
import os

from array import array
from collections import defaultdict
from pathlib import Path
from typing import Dict, List

from kftlib.event import KFTEvent, TdEvents
from kftlib.elf import Elf


def inspect_kft_file(path: Path,
                     elf: Elf,
                     td_max: int = 180) -> TdEvents:
    """
    Inspect kft dump and return events for each running thread.

    Arguments:
        path -- path to .kft file
        kern_start -- starting address of the kernel (from elf that was run to
                      obtain dump)
        td_max -- maximal number of threads (default: 180)

    Returns:
        dictionary of events for each thread (indexed with thread id)
    """
    events: Dict[int, List[KFTEvent]] = defaultdict(list)
    td_time = [0] * (td_max + 1)  # elapsed time
    cur_thread = -1
    cur_time = 0
    switch_time = 0
    ctx_swith_count = 0

    entries = array('Q')
    with open(path, 'rb') as f:
        entries.frombytes(f.read())

    print(f'Read {len(entries)} KFT events')

    for i, v in enumerate(entries):
        thread, event = KFTEvent.decode(v, elf.kernel_start)

        if thread != cur_thread:
            # update info about prev thread
            td_time[cur_thread] += event.timestamp - switch_time

            # save values for current thread
            switch_time = event.timestamp
            if thread > td_max:
                raise IndexError('There are too many threads.'
                                 'Try to increase td_max.')
            cur_time = td_time[thread]
            cur_thread = thread
            ctx_swith_count += 1

        # Update event to the time elapsed when current thread was running.
        event.timestamp = cur_time + (event.timestamp - switch_time)

        events[cur_thread].append(event)

    return events
