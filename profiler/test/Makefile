.PHONY: all clean

all: test

test: main.c ../profiler.h ../profiler.c Makefile
	$(CC) -DTEST_PROFILER -g -finstrument-functions -finstrument-functions-exclude-file-list=../profiler.h,../profiler.c --include=../profiler.h $< ../profiler.c -o $@

clean:
	rm test
