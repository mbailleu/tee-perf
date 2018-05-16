#/usr/bin/env python3

import numpy as np
import pandas as pd
import struct
import subprocess
from io import *
from typing import Tuple, List
from collections import defaultdict
import argparse
import scone_dump_elf as de
import sys
import re
from threading import Thread

nsec_t = "u8"
pid_t = "u8"
ptr_t = "u8"
size_t = "u8"

SCONE = True
SHOW_STACK = False
data = None
elf_file = ""
log_file = ""

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
                           ("callee", ptr_t)])
        header_t = np.dtype([("nsec", nsec_t),
                             ("self", ptr_t),
                             ("pid", pid_t),
                             ("size", size_t),
                             ("data", ptr_t)])
        header = np.frombuffer(buf, dtype=header_t, count=1)
        size = header["data"] - header["self"] - header_t.itemsize
        max_size = header["size"] - header_t.itemsize
        size = min(size, max_size)
        data = np.frombuffer(buf, dtype=data_t, offset=header_t.itemsize, count=int(size//data_t.itemsize))
        return (header, pd.DataFrame(data))
    except IOError:
        print("Could not read file: ", filename)
        sys.exit(1)

def addr2line(binary: str, column) -> List[Tuple[str,str]]:
    def write_to(column, stdin):
        for entry in column:
            stdin.write(hex(entry).encode("utf8"))
            stdin.write(b"\n")
#            print(hex(entry), file=stdin)
        stdin.flush()
        stdin.close()

    args = ["addr2line", "-e", binary, "-f"]
    process = subprocess.Popen(args, stdout=subprocess.PIPE, stdin=subprocess.PIPE)
    Thread(target=write_to, args=(column, process.stdin)).start()
    res = []
    val = None
    i = 0
    for line in process.stdout:
        line = line.decode("utf8").rstrip()
        if i == 1:
            res.append((val,line))
            i = 0
        else:
            val = line
            i = 1
    return res

def readelf_find_addr(binary: str, funcs: List[str]) -> List[int]:
    args = ["readelf", "-s", binary]
    patterns = [re.compile(" " + func + "$") for func in funcs]
    process = subprocess.Popen(args, stdout=subprocess.PIPE)
    res = []
    for line in process.stdout:
        line = line.decode("utf8").rstrip()
        if any(map(lambda pattern: pattern.search(line), patterns)):
            line = line.lstrip()
            vals = line.split(" ")
            res.append(int(vals[1], 16))
    return res

def lazy_function_name(data, elf_file):
    print("Get function names")
    entries = data["callee"].drop_duplicates()
    addr = readelf_find_addr(elf_file, ["main"])
    mask = (1 << 64) - 1
    def test_mask(entry):
        return any(map(lambda a: entry & mask == a, addr))

    while not entries.map(test_mask).any() and mask != 0:
            mask = mask >> 1
    masked_entries = entries.map(lambda e: e & mask)
    res = addr2line(elf_file, masked_entries)
    for e, r in zip(entries, res):
        data.at[data.callee == e, "callee_name"] = r[0]

def get_db(file_name: str, elf_file: str):
#    file_name = "/tmp/__profiler_file_scone.shm"
#    elf_file = "../profiler/test/test"
    global SCONE
    print("Read File:", file_name)
    header, data = readfile(file_name)
    direction_mask = 1 << 63
    time_mask = direction_mask - 1
    
    data["direction"] = data["nsec"].map(lambda x: 1 if (x & direction_mask) else 0)
    data["time"] = data["nsec"].map(lambda x:  x & time_mask)

    data.drop(["nsec"], axis=1, inplace=True)

    return data

def show_func_call(depth: int, name: str):
    global SHOW_STACK
    if SHOW_STACK == True:
        print("| " * depth,"-> ",name,sep='')

def build_stack(data):
    print("build stack")
    stack_depth = 0
    stack = []   #(idx,time,[])
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
            idx, t, prev, tmp_caller = stack.pop()
            stack_list["idx"].append(idx)
            stack_list["time"].append(int(row[2] - t - prev_time))
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

def show_times(data):
    def print_times():
        print(data.groupby(["callee_name"])[["callee_name","time","percent"]].sum().sort_values(by=["time"], ascending=False))

    with pd.option_context("display.max_rows", None, "display.max_columns", 3, "display.float_format", "{:.4f}".format):
        try:
            print_times()
        except KeyError:
            global elf_file
            lazy_function_name(data, elf_file)
            print_times()
        
def main():
    global INTERACTIVE
    args = parse_args()
    set_globals(args)
    dump_output(args) 
    global data
    data = get_db(args.profile_dump, args.elf_file)
    data = build_stack(data)
    data["percent"] = (data["time"] / data["time"].sum()) * 100
    show_times(data)

if __name__ == "__main__":
    main()

def find_callers(func: str, data = data):
    callers = pd.merge(data[data.callee_name == func].caller.to_frame(), data, left_on="caller", right_on="idx", how="inner")["callee_name"].value_counts()
    print(callers)

def count_calls(func: str, data = data):
    print(data[data.callee_name == func]["callee_name"].count())
