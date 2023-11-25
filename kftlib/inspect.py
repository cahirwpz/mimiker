from __future__ import annotations

import logging
import os

from pathlib import Path
from typing import Dict, List

from .event import KFTEvent, TdEvents
from .elf import Elf


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
    st = os.stat(path)
    size = st.st_size

    # File should contain kft entries which have 8B size
    assert size % 8 == 0
    entries = int(size / 8)
    mb_size = size / 1024 / 1024
    logging.info(f'Reading file of size {mb_size}MB' f'({entries} entries)')

    events: Dict[int, List[KFTEvent]] = dict()
    td_time = [0] * (td_max + 1)  # elapsed time
    cur_thread = -1
    cur_time = 0
    switch_time = 0
    ctx_swith_count = 0

    file = open(path, 'rb', buffering=800)
    for i in range(entries):
        v = int.from_bytes(file.read(8), 'little')
        thread, event = KFTEvent.decode(v, elf.kernel_start)

        if thread != cur_thread:
            # update info about prev thread
            td_time[cur_thread] += event.timestamp - switch_time

            if thread not in events:
                events[thread] = []

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
