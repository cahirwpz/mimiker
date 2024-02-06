import logging

from collections import defaultdict
from typing import Dict, List, Any

from kftlib.event import KFTEvent, TdEvents, KFTEventType

FTimes = Dict[int, List[int]]

ListDict = Dict[Any, List[Any]]


def _get_times(event_list: List[KFTEvent], pc_list: List[int]) -> FTimes:
    fn_times: FTimes = defaultdict(list)
    last_event = {}

    for pc in pc_list:
        last_event[pc] = (KFTEventType.KFT_OUT, 0)

    for e in event_list:
        if e.pc not in pc_list:
            continue

        last_type = last_event[e.pc][0]
        last_time = last_event[e.pc][1]

        if last_type == e.typ:
            logging.debug(f"PC: {e.pc} -- recursion")

        if e.typ == KFTEventType.KFT_OUT:
            elapsed = e.timestamp - last_time
            fn_times[e.pc].append(elapsed)

        last_event[e.pc] = (e.typ, e.timestamp)

    for pc, event in last_event.items():
        if event[0] == KFTEventType.KFT_IN:
            logging.debug(f"PC: {pc} -- haven't returned")

    return fn_times


def get_functions_times(events: TdEvents, pc_list: List[int]) -> FTimes:
    """
    Get list of execution times for each function from given list.

    Arguments:
        events -- events read from kft dump
        pc_list -- list of pcs of functions to analyze
    """
    fn_times: FTimes = defaultdict(list)

    for td_events in events.values():
        for pc, times in _get_times(td_events, pc_list).items():
            fn_times[pc].extend(times)

    return fn_times
