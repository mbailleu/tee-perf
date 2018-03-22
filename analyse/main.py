#/usr/bin/env python3

import numpy as np
import pandas as pd
import struct
import subprocess
from io import *
from typing import Tuple, List

sec_t = "u8"
nsec_t = "u8"
pid_t = "u8"
ptr_t = "u8"
size_t = "u8"

SCONE = True
elf_file = "../profiler/test/test"

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

file_name = "/tmp/__profiler_file_scone.shm"
header, data = readfile(file_name)

scone_force = clean_addr(0, data)

for callee in data["callee"]:
    print(hex(callee))

get_names(elf_file, data, "callee", "callee_name", "callee_file")

if SCONE == True and data["callee_name"].mode().any() == "??":
    scone_force = clean_addr(scone_force, data)
    get_names(elf_file, data, "callee", "callee_name", "callee_file")
    if data["calle_name"].mode().any() == "??":
        print("Could probably not detect right elf format, assuming: " + force_to_str(scone_force))

get_names(elf_file, data, "caller", "caller_name", "caller_file")

print(data)
