#/usr/bin/env python3

import argparse
import subprocess
from typing import List

def exec_docker(args: List[str]):
    program = ["sudo", "docker"]
    program.extend(args)
    subprocess.check_output(program)

def dump(container_name: str, executable: str, output: str):
    dump_file = "/tmp/scone_dump_elf"
    exec_docker(["cp", executable, container_name+":/tmp/"+executable])
    gdb = ["exec", container_name, "scone-gdb", "-nx", "--batch"]
    for cmd in ["break main", "run", "continue", "shell cp $SCONE_DUMP_DEBUG "+dump_file+"; exit"]:
        gdb.extend(["-ex". cmd])
    exec_docker(gdb)
    exec_docker(["cp", container_name+":"+dump_file, output])
    exec_docker(["exec", container_name, "rm", "/tmp/"+executable, dump_file])

def main():
    parser = argparse.ArgumentParser(description="Dumps the encalve ELF from Scone")
    parser.add_argument("container_name", type=str, help="Scone docker container with scone-gdb")
    parser.add_argument("executable", type=str, help="With scone compiled executable, which enclave ELF should be dumped")
    parser.add_argument("-o", "--output", type=str, default=None, help="Dump of the scone compiled executable, if not given assuming 'executable'.sconeelf")
    args = parser.parse_args()
    output = None
    if args.output is None:
        output = args.executable + ".sconeelf"
    else:
        output = args.output
    dump(args.container_name, args.executable, output)

if __name__ == "__main__":
    main()
