## Getting started

### Dependencies
* gcc
* scone-gcc
* scone-gdb
* \>= python 3.6
* numpy
* pandas
* argparse
* addr2line
* readelf
* make
* c++filt
* progressbar2
* flamegraph

### How To Use
1. Compile the timer application under profiler/ with make

2. Recompile the application to be profiled with the following additional gcc flags:
```
-finstrument-functions --include=path/to/profiler.h/in/profiler/
```

   You have to compile the profiler.c with scone-gcc and link the object file into the execuable

3. Run the timer with the application as argument
```
./timer /path/to/measurment/file Size_of_measurment_file application application argument(s)
```

4. In analyse run the main.py with the elf-file executed inside the enclave and the perf/file.

## Limitation
* In contrast to the normal perf tool, this tool does not sample the application but records each function call/return. This is very memory consuming. In my experience a 2GiB log file can store around 20s of execution (This was without compression. Compression reduced the record size from 40B per record to 16B.).
* If the measurment file is full the application will `SEGFAULT`. Currently, this is a wanted behavior, as it is an easy way to end the application, when the measurment is over. However, I am not decided on that yet.
* Analysing the measurment file can take some time. For a full 2GiB measurment file this can be around 4 minutes on an Intel Xeon E3-1270. The analyser is mostly single threaded, therefore the single thread performance of the CPU is the limiting factor
* Analysing takes also a huge amount of memory. The mentioned 2GiB measurment file resulted in up to 20GiB of memory requirement while analysing. However, I worked to reduce the necessary amount of memory and have not test it since. Thus this number could be a lot smaller now.

## Example
In the `profiler` directory is a `test` directory. Compile with:
```
make SCONECC=scone-gcc
```
Run in the `profiler` directory
```
./timer /tmp/__measurement_shm 4k ./test/test hello world
```
This should result in the following output:
```
HERE
HERE
HERE
HERE
HERE
HERE
HERE
HERE
HERE
HERE
HERE
HERE
./test/test
hello
world
```
After this switch to the `analyse` directory and run:
```
python3 main.py -d container_with_scone-gdb ../profiler/test/test "dump" "/tmp/__measurement_shm"
```
The output should look similar to this:
```
835     ./tools/starter-exec.c: No such file or directory.
could not find SGXPROFILERSHM enviroment variableDump enclave ELF to perf_dump
Read File: /tmp/__measurement_shm
build stack
Get function names
             time  percent
callee_name               
a            3465  52.9250
print_args   1685  25.7370
aba          1381  21.0936
b              11   0.1680
main            5   0.0764
```
To run the analyses again you can just run:
```
python3 main.py dump "/tmp/__measurement_shm"
```

If you run the main.py with the python argument -i you can also work further on the data. Two convenience methods currently exist:
```python3
find_callers(function_name: str)
count_call(function_name: str)
```
