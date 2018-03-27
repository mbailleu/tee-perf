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
from threading import Thread


sec_t = "u8"
nsec_t = "u8"
pid_t = "u8"
ptr_t = "u8"
size_t = "u8"

SCONE = True
SHOW_STACK = False
INTERACTIVE = False

class SI_prefix:
    def __init__(self, numerator: int, denominator: int):
        self.numerator = numerator
        self.denominator = denominator

milli = SI_prefix(1, 10 ** 3)
micro = SI_prefix(1, 10 ** 6)
nano  = SI_prefix(1, 10 ** 9)

def readfile(filename: str) -> Tuple:
    buf = open(filename, 'rb').read()
    data_t = np.dtype([("sec", sec_t),
                       ("nsec", nsec_t),
                       ("callee", ptr_t),
                       ("caller", ptr_t),
                       ("direction", "u8")])
    header_t = np.dtype([("sec", sec_t),
                         ("nsec", nsec_t),
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

def clean_addr(force: int, data) -> int:
    scone_offset = 0x1000000000
    if SCONE == True and force < 2 and (force == 1 or data["callee"].min() >= scone_offset): 
        data["callee"] = data["callee"] - scone_offset
        data["caller"] = data["caller"] - scone_offset
        return 2
    if force == 2:
        data["callee"] = data["callee"] + scone_offset
        data["caller"] = data["caller"] + scone_offset
        return 3
    if SCONE == True:
        return 1
    return 3

def get_names(file: str, data, key: str, func_name: str, file_name: str):
    if len(data.index) > 0:
        res = addr2line(file, data[key])
        data[func_name] = [func for func, file in res]
        data[file_name] = [file for func, file in res]

def force_to_str(force: int) -> str:
    if (force == 0):
        return "No transformation"
    if (force == 1):
        return "Non-Scone ELF"
    if (force == 2):
        return "Scone ELF"
    if (force == 3):
        return "Non-Scone ELF"

def get_db(file_name: str, elf_file: str):
#    file_name = "/tmp/__profiler_file_scone.shm"
#    elf_file = "../profiler/test/test"
    print("Read File:", file_name)
    header, data = readfile(file_name)
    
    data["time"] = data["sec"] * nano.denominator + data["nsec"]

    data.drop(["sec","nsec"], axis=1, inplace=True)

    scone_force = clean_addr(0, data)
    
    print("Get callee functions")
    get_names(elf_file, data, "callee", "callee_name", "callee_file")
    
    if SCONE == True and data["callee_name"].mode().any() == "??":
        scone_force = clean_addr(scone_force, data)
        get_names(elf_file, data, "callee", "callee_name", "callee_file")
        if data["callee_name"].mode().any() == "??":
            print("Could probably not detect right elf format, assuming: " + force_to_str(scone_force))
    
    if False:
        print("Get caller functions")
        get_names(elf_file, data, "caller", "caller_name", "caller_file")

    return data

def show_func_call(depth: int, name: str):
    if SHOW_STACK == True:
        print("| " * depth,"-> ",name,sep='')

def build_stack(data):
    print("build stack")
    stack_depth = 0
    stack = []   #(idx,time,[])
    stack_list = defaultdict(list)
    prev_time = 0
    for row in data[["direction","time","callee_name"]].itertuples():
        if int(row[1]) == 0:
            stack.append((row[0],row[2],prev_time))
            prev_time = 0
            stack_depth += 1
            show_func_call(stack_depth, row[3])
        else:
            stack_depth -= 1
            idx, t, prev = stack.pop()
            stack_list["idx"].append(idx)
            stack_list["time"].append(int(row[2] - t - prev_time))
            stack_list["depth"].append(stack_depth)
            stack_list["callee_name"].append(row[3])
            prev_time = prev + int(row[2] - t)
#    import pdb; pdb.set_trace()
    return pd.DataFrame(stack_list, index=stack_list["idx"])

def find_callers(data, func: str):
    tmp = data.sort_values(by=["idx"], ascending=False)
    lists = defaultdict(list)
    for entry in tmp[tmp["callee_name"] == func][["idx", "depth"]].itertuples():
        for e in tmp[tmp.index < entry[0]][["callee_name", "depth"]].itertuples():
            if e[2] < entry[2]:
                lists["callee_name"].append(e[1])
                break
    print(pd.DataFrame(lists)["callee_name"].value_counts())

def parse_args():
    parser = argparse.ArgumentParser(description="Reads profile dump from a Scone profiler generated file")
    parser.add_argument("elf_file", metavar="elf-file", type=str, help="Elf file for parsing symbols")
    parser.add_argument("profile_dump", metavar="profile-dump", type=str, help="Profiler dump file")
    parser.add_argument("-ns", "--no-scone", action="store_true", help="Try not scone elf parsing")
    parser.add_argument("-s", "--show-stack", action="store_true", help="Show Stack")
    parser.add_argument("-i", "--interactive", action="store_true", help="Get a interactive shell at the end")
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
    global INTERACTIVE
    if args.no_scone == False:
        SCONE = True
    else:
        SCONE = False
    SHOW_STACK = args.show_stack
    INTERACTIVE = args.interactive


def main():
    args = parse_args()
    set_globals(args)
    dump_output(args)
    data = get_db(args.profile_dump, args.elf_file)
    data = build_stack(data)
    #print(data)
    data["percent"] = (data["time"] / data["time"].sum()) * 100
    with pd.option_context("display.max_rows", None, "display.max_columns", 3, "display.float_format", "{:.4f}".format): 
            print(data.groupby(["callee_name"])[["callee_name","time","percent"]].sum().sort_values(by=["time"], ascending=False))
   # find_callers(data, "a")
    if INTERACTIVE:
        import pdb; pdb.set_trace()

if __name__ == "__main__":
    main()

