#/usr/bin/env python3

import numpy as np
import pandas as pd
import struct
import subprocess
from io import *
from typing import Tuple, List
import argparse
import scone_dump_elf as de

sec_t = "u8"
nsec_t = "u8"
pid_t = "u8"
ptr_t = "u8"
size_t = "u8"

SCONE = True

def readfile(filename: str) -> Tuple:
    buf = open(filename, 'rb').read()
    data_t = np.dtype([("sec", sec_t),
                       ("nsec", nsec_t),
                       ("callee", ptr_t),
                       ("caller", ptr_t),
                       ("direction", "u8")])
    header_t = np.dtype([("self", ptr_t),
                         ("sec", sec_t),
                         ("nsec", nsec_t),
                         ("pid", pid_t),
                         ("size", size_t),
                         ("data", ptr_t)])
    header = np.frombuffer(buf, dtype=header_t, count=1)
    size = header["data"] - header["self"] - header_t.itemsize
    data = np.frombuffer(buf, dtype=data_t, offset=header_t.itemsize, count=int(size//data_t.itemsize))
    return (header, pd.DataFrame(data))

def addr2line(binary: str, entry) -> List[Tuple[str,str]]:
    args = ["addr2line", "-e", binary, "-f"]
    for e in entry:
        args.append(hex(e))
    process = subprocess.Popen(args, stdout=subprocess.PIPE)
    lines = process.communicate()[0].decode("utf-8").splitlines()
    process.wait()
    res = []
    for i in range(0,len(lines),2):
        res.append((lines[i],lines[i+1]))
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
    header, data = readfile(file_name)
    
    scone_force = clean_addr(0, data)
    
    get_names(elf_file, data, "callee", "callee_name", "callee_file")
    
    if SCONE == True and data["callee_name"].mode().any() == "??":
        scone_force = clean_addr(scone_force, data)
        get_names(elf_file, data, "callee", "callee_name", "callee_file")
        if data["calle_name"].mode().any() == "??":
            print("Could probably not detect right elf format, assuming: " + force_to_str(scone_force))
    
    get_names(elf_file, data, "caller", "caller_name", "caller_file")
    return data

def build_stack(data):
    i = 0
    stack_depth = []
    for row in data["direction"]:
        stack_depth.append(i)
        if int(row) == 0:
            i += 1
        else:
            i -= 1
    data["stack_depth"] = stack_depth
#    import pdb; pdb.set_trace()
    show_stack(data)

def show_stack(data):
    def apply(row):
        if row["stack_depth"] == 0:
            print(row["callee_name"])
            return
        for i in range(int(row["stack_depth"])):
            print("| ", end="")
        print("->",row["callee_name"])
    data[data["direction"] == 0].apply(apply, axis=1)

def main():
    parser = argparse.ArgumentParser(description="Reads profile dump from a Scone profiler generated file")
    parser.add_argument("elf_file", metavar="elf-file", type=str, help="Elf file for parsing symbols")
    parser.add_argument("profile_dump", metavar="profile-dump", type=str, help="Profiler dump file")
    parser.add_argument("-ns", "--no-scone", action="store_true", help="Try not scone elf parsing")
    parser.add_argument("-d", "--dump", nargs=2, help="Also dump target enclave ELF <scone container> <executable>")
    parserdump = parser.add_argument_group("Arguments for dumping enclave ELF")
    parserdump.add_argument("-do", "--dump-output", type=str, default=None, help="Dump of the scone compiled executable, if not given assuming elf_file")
    args = parser.parse_args()
    if args.no_scone == False:
        SCONE = True
    else:
        SCONE = False
    if args.dump is not None:
        dump_output = None
        if args.dump_output is None:
            dump_output = args.elf_file
        else:
            dump_output = args.dump_output
        if SCONE == True:
            de.dump(args.dump[0], args.dump[1], dump_output)
        else:
            print("Cannot dump Scone ELF without Scone")
    data = get_db(args.profile_dump, args.elf_file)
    build_stack(data)

if __name__ == "__main__":
    main()

