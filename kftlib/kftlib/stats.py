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


def _dict_append(d: ListDict, other: ListDict) -> ListDict:
    for k, v in other.items():
        if k in d:
            d[k] += (v)
        else:
            d[k] = v


def get_functions_times(events: TdEvents, pc_list: List[int]) -> FTimes:
    """
    Get list of execution times for each function from given list.

    Arguments:
        events -- events read from kft dump
        pc_list -- list of pcs of functions to analyze
    """
    fn_times: FTimes = {}

    for td_events in events.values():
        _dict_append(fn_times, _get_times(td_events, pc_list))

    return fn_times
