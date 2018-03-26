.PHONY: all clean

all: test timer

timer: ../timer.c ../profiler_data.h Makefile
	$(HOSTCC) -g -lpthread $< -o $@

test: main.c ../profiler.h ../profiler_data.h ../profiler.c Makefile
	$(SCONECC) -static -DTEST_PROFILER -g -finstrument-functions -finstrument-functions-exclude-file-list=../profiler.h,../profiler.c --include=../profiler.h $< ../profiler.c -o $@

clean:
	rm test
