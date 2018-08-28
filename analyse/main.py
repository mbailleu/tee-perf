#/usr/bin/env python3

import numpy as np      # type: ignore
import pandas as pd     # type: ignore
import struct
import subprocess
from io import *
from typing import Tuple, List, Iterable, TypeVar, Callable, IO, Any
from collections import defaultdict
import argparse
import scone_dump_elf as de
import sys
import re
from threading import Thread
from itertools import zip_longest
from multiprocessing import Pool
from functools import reduce
import time
import progressbar

nsec_t = "u8"
flags_t = "u8"
pid_t = "u8"
ptr_t = "u8"
size_t = "u8"
tid_t = "u8"

SCONE = True
SHOW_STACK = False
data = None
elf_file = ""
log_file = ""
mask = 0

#class SI_prefix:
#    def __init__(self, numerator: int, denominator: int):
#        self.numerator = numerator
#        self.denominator = denominator

#milli = SI_prefix(1, 10 ** 3)
#micro = SI_prefix(1, 10 ** 6)
#nano  = SI_prefix(1, 10 ** 9)

def readfile(filename: str) -> Tuple:
    try:
        buf = open(filename, 'rb').read()
        data_t = np.dtype([("nsec", nsec_t),
                           ("callee", ptr_t),
                           ("thread_id", tid_t)],
                           )
        header_t = np.dtype([("nsec", nsec_t),
                             ("flags", flags_t),
                             ("self", ptr_t),
                             ("pid", pid_t),
                             ("size", size_t),
                             ("data", ptr_t),
                             ("bin_location", ptr_t)])
        header = np.frombuffer(buf, dtype=header_t, count=1)
        size = header["data"] - header["self"] - header_t.itemsize
        max_size = header["size"] - header_t.itemsize
        size = min(size, max_size)
        data = np.frombuffer(buf, dtype=data_t, offset=header_t.itemsize, count=int(size//data_t.itemsize))
        return (header, pd.DataFrame(data))
    except IOError:
        print("Could not read file: ", filename)
        sys.exit(1)

T = TypeVar("T")
Pipe_t = TypeVar("Pipe_t")
Res_t = TypeVar("Res_t")
def call_app(app: List[str], args: Iterable[T], map_write: Callable[[T],str], res: Callable[[IO[Any]], Res_t]) -> Res_t:
    def write_to(args: Iterable, stdin, map_write) -> None:
        for entry in args:
            stdin.write(map_write(entry).encode("utf8"))
            stdin.write(b"\n")
        stdin.flush()
        stdin.close()

    process = subprocess.Popen(app, stdout=subprocess.PIPE, stdin=subprocess.PIPE)
    Thread(target=write_to, args=(args, process.stdin, map_write)).start()
    return res(process.stdout)

def demangle(methods: Iterable[str]) -> List[str]:
    return call_app(["c++filt"], methods, lambda x: x, lambda stdout: [line.decode("utf8").rstrip() for line in stdout])


def addr2line(binary: str, column) -> List[Tuple[str,str]]:
    return call_app(["addr2line", "-e", binary, "-f"], column, hex,
                        lambda stdout: [(func.decode("utf8").rstrip(), file.decode("utf8").rstrip()) 
                                            for func, file in zip_longest(*[stdout]*2)])

def readelf_find_addr(binary: str, funcs: List[str]) -> List[int]:
    patterns = [re.compile(" " + func + "$") for func in funcs]
    process = subprocess.Popen(["readelf", "-s", binary], stdout=subprocess.PIPE)
    res = []
    for line in process.stdout:
        line = line.decode("utf8").rstrip()
        if any(map(lambda pattern: pattern.search(line), patterns)):
            line = line.lstrip()
            vals = line.split(" ")
            res.append(int(vals[1], 16))
    return res

def lazy_function_name(data, elf_file: str) -> None:
    print("Get function names")
    print("Drop duplicates")
    entries = data["callee"].drop_duplicates()
    print("Find addresses")
    addr = readelf_find_addr(elf_file, ["main"])
    global mask
    print("Apply mask")
    masked_entries = entries.map(lambda e: e & mask)
    print("Find function names")
    func_name = addr2line(elf_file, masked_entries)
    print("Demangle function names")
    tmp = pd.DataFrame({"callee_name" : demangle([f[0] for f in func_name])}, index = entries )
    return data.merge(tmp, left_on="callee", right_index=True, how="left")     

def get_function_mask(header, elf_file: str):
    print("Find relocation")
    addr = readelf_find_addr(elf_file, ["__profiler_map_info"])
    print("Find mask")
    global mask
    mask = (1 << 64) - 1
    while (mask & header["bin_location"]) != addr and mask != 0:
        mask = mask >> 1

def get_db(file_name: str, elf_file: str):
    global SCONE
    print("Read File:", file_name)
    header, data = readfile(file_name)
    direction_mask = 1 << 63
    time_mask = direction_mask - 1
    
    data["direction"] = data["nsec"].map(lambda x: 1 if (x & direction_mask) else 0)
    data["time"] = data["nsec"].map(lambda x:  x & time_mask)

    data.drop(["nsec"], axis=1, inplace=True)

    get_function_mask(header, elf_file)

    import pdb; pdb.set_trace()
    return data

def show_func_call(depth: int, name: str) -> None:
    global SHOW_STACK
    if SHOW_STACK == True:
        print("| " * depth,"-> ",name,sep='')

def build_stack(thread_id, data):
    print("build stack of thread:", hex(thread_id))
    stack_depth = 0
    stack = [] # type: List[Tuple[int, int, int, int]]
    stack_list = defaultdict(list)
    prev_time = 0
    caller = -1
    for row in data[["direction","time","callee"]].itertuples():
        if int(row[1]) == 0:
            stack.append((row[0],row[2],prev_time,caller))
            caller = row[0]
            prev_time = 0
            stack_depth += 1
            show_func_call(stack_depth, row[3])
        else:
            stack_depth -= 1
            if not stack:
                continue
            idx, t, prev, tmp_caller = stack.pop()
            stack_list["idx"].append(idx)
            stack_list["time_d"].append(int(row[2] - t - prev_time))
            stack_list["depth"].append(stack_depth)
            stack_list["callee"].append(row[3])
            stack_list["caller"].append(tmp_caller)
            prev_time = prev + int(row[2] - t)
            caller = tmp_caller
    return pd.DataFrame(stack_list, index=stack_list["idx"])


def parse_args():
    parser = argparse.ArgumentParser(description="Reads profile dump from a Scone profiler generated file")
    parser.add_argument("elf_file", metavar="elf-file", type=str, help="Elf file for parsing symbols")
    parser.add_argument("profile_dump", metavar="profile-dump", type=str, help="Profiler dump file")
    parser.add_argument("-ns", "--no-scone", action="store_true", help="Try not scone elf parsing")
    parser.add_argument("-s", "--show-stack", action="store_true", help="Show Stack not recommended for bigger logs")
    parser.add_argument("-d", "--dump", nargs=2, help="Also dump target enclave ELF <scone container> <executable>")
    parserdump = parser.add_argument_group("Arguments for dumping enclave ELF")
    parserdump.add_argument("-do", "--dump-output", type=str, default=None, help="Dump of the scone compiled executable, if not given assuming elf_file")
    return parser.parse_args()

def dump_output(args):
    if args.dump is not None:
        dump_output = None
        if args.dump_output is None:
            dump_output = args.elf_file
        else:
            dump_output = args.dump_output
        if SCONE == True:
            print("Dump enclave ELF to", dump_output)
            de.dump(args.dump[0], args.dump[1], dump_output)
        else:
            print("Cannot dump Scone ELF without Scone")

def set_globals(args):
    global SCONE
    global SHOW_STACK
    global elf_file
    global file_name
    if args.no_scone == False:
        SCONE = True
    else:
        SCONE = False
    SHOW_STACK = args.show_stack
    elf_file = args.elf_file
    log_file = args.profile_dump

def show_times(thread_id, data, percent: str):
    def print_times():
        print("\nThread:", hex(thread_id))
        print(data.groupby(["callee_name"])[["callee_name","time_d", percent]].sum().sort_values(by=["time_d"], ascending=False))
        print()

    with pd.option_context("display.max_rows", None, "display.max_columns", 3, "display.float_format", "{:.4f}".format):
            print_times()


def stack_loop(t):
    thread_id, thread = t
    thread = build_stack(thread_id, thread)
    thread["percent"] = (thread["time_d"] / thread["time_d"].sum()) * 100
    return (thread_id, thread)

class IterableFromTuples:
    def __init__(self, res, idx):
        self.tuple = res
        self.idx = idx

    def __iter__(self):
        self.iter = self.tuple.__iter__()
        return self

    def __next__(self):
        res = self.iter.__next__()
        return res[self.idx]
        
def main():
    global INTERACTIVE
    args = parse_args()
    set_globals(args)
    dump_output(args) 
    global data
    data = get_db(args.profile_dump, args.elf_file)
    data = lazy_function_name(data, elf_file)
    #pool = Pool()
    #res = pool.map(stack_loop, data.groupby("thread_id"))
    res = [stack_loop(x) for x in data.groupby("thread_id")]
    threads = pd.concat(IterableFromTuples(res, 1))
    data = data.merge(threads[["time_d", "depth", "percent", "caller"]], right_index = True, left_index = True)
    for thread_id, thread in data.groupby("thread_id"):
        show_times(thread_id, thread, "percent")
    data["acc_percent"] = (data["time_d"] / data["time_d"].sum()) * 100
    show_times(0, data, "acc_percent")
        

if __name__ == "__main__":
    main()

def find_callers(func: str, data = data) -> None:
    callers = pd.merge(data[data.callee_name == func].caller.to_frame(), data, left_on="caller", right_index=True, how="inner")["callee_name"].value_counts()
    print(callers)

def count_calls(func: str, data = data) -> None:
    print(data[data.callee_name == func]["callee_name"].count())

def find_source(func: str, data = data, elf_file: str = elf_file) -> None:
    global mask
    callees = []
    for callee, rest in data[data.callee_name == func].groupby("callee"):
        callees.append(callee & mask)
    res = addr2line(elf_file, callees)
    for func, src in res:
        print(src)
