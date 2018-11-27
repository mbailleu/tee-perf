#!/usr/bin/env python3

import numpy as np
import pandas as pd

from typing import Tuple, List, Iterable, TypeVar, Callable, IO, Any, AnyStr

def get_stack(data, caller) -> str:
    if (int(caller) == -1):
        return ""
    for entry in data[data.index == caller][["caller", "callee_name"]].itertuples():
        return get_stack(data, entry.caller) + entry.callee_name + ';'

def export_to(out: Callable[[bool], AnyStr], data) -> None:
    for callee in data.callee_name.unique():
        for entry in data[data.callee_name == callee][["caller","time_d"]].groupby("caller").sum().itertuples():
            if not out(get_stack(data, entry.Index) + callee + ' ' + str(entry.time_d) + '\n'):
                return
