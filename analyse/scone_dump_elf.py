#/usr/bin/env python3

import os
import argparse
import subprocess
from typing import List

def exec_docker(args: List[str]) -> None:
    program = ["sudo", "docker"]
    program.extend(args)
    subprocess.check_output(program)

def exec_docker_cp_workaround_to(container_name: str, executable: str, exec_file: str) -> None:
    f = open(executable, "rb")
    subprocess.check_output(["sudo", "docker", "exec", "-i", container_name, "bash", "-c", "cat > "+exec_file], stdin=f)
    f.close()

def exec_docker_cp_workaround_from(container_name: str, exec_file: str, output: str) -> None:
    f = open(output, "wb")
    docker = subprocess.Popen(["sudo", "docker", "exec", container_name, "cat", exec_file], stdout=f)
    docker.wait()
    f.close()

def dump(container_name: str, executable: str, output: str, in_container: bool) -> None:
    dump_file = "/tmp/scone_dump_elf"
    if in_container == True:
        exec_file = executable
    else:
        exec_file = "/tmp/"+os.path.basename(executable)
        executable = os.path.abspath(executable)
        exec_docker_cp_workaround_to(container_name, executable, exec_file)
    exec_docker(["exec", container_name, "chmod", "+x", exec_file])
#    exec_docker(["cp", executable, container_name+":"+exec_file])
    gdb = ["exec", container_name, "scone-gdb", "-nx", "--batch"]
    for cmd in ["break main", "run", "continue", "shell cp $SCONE_DUMP_DEBUG "+dump_file+"; exit"]:
        gdb.extend(["-ex", cmd])
    gdb.append(exec_file)
    exec_docker(gdb)
#    exec_docker(["cp", container_name+":"+dump_file, output])
    exec_docker_cp_workaround_from(container_name, dump_file, output)
    if in_container == False:
        exec_docker(["exec", container_name, "rm", exec_file, dump_file])

def main():
    parser = argparse.ArgumentParser(description="Dumps the encalve ELF from Scone")
    parser.add_argument("container_name", type=str, help="Scone docker container with scone-gdb")
    parser.add_argument("executable", type=str, help="With scone compiled executable, which enclave ELF should be dumped")
    parser.add_argument("-o", "--output", type=str, default=None, help="Dump of the scone compiled executable, if not given assuming 'executable'.sconeelf")
    parser.add_argument("--in-container", action="store_true", help="The executable is already in the container [WHY JUST WHY????]")
    args = parser.parse_args()
    output = None
    if args.output is None:
        output = args.executable + ".sconeelf"
    else:
        output = args.output
    dump(args.container_name, args.executable, output, args.in_container)

if __name__ == "__main__":
    main()
