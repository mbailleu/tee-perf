#!/usr/bin/env python3

import numpy as np
import pandas as pd

from typing import Tuple, List, Iterable, TypeVar, Callable, IO, Any, AnyStr

def get_stack(data, caller) -> str:
    if (int(caller) == -1):
        return ""
    for method_name in data[data.index == caller].callee_name:
        return get_stack(data, data[data.index == caller].groupby("caller")) + method_name

def export_to(data) -> [str]:
    res : [str] = []
    for callee in data.callee_name.unique():
        for caller in data[data.callee_name == callee].groupby("caller").time_d.sum():
            stack = get_stack(data, caller)
            import pdb; pdb.set_trace()
            print(stack)
            print(callee)
            print(caller.time_d)
            res.append(get_stack(data, caller) + ';' + callee + " " + caller.time_d)
    return res

