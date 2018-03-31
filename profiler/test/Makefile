.PHONY: all clean

SCONECC ?= $(CC)
HOSTCC ?= $(CC)

all: test timer

timer: ../timer.c ../profiler_data.h Makefile
	$(HOSTCC) -g -lpthread $< -o $@

test: main.o profiler.o libaba.a
	$(SCONECC) -static -DTEST_PROFILER -g -finstrument-functions main.o profiler.o -o $@ -L./ -laba

main.o: main.c ../profiler.h ../profiler_data.h aba.h Makefile
	$(SCONECC) -static -DTEST_PROFILER -g -finstrument-functions --include=../profiler.h -c $< -o $@

profiler.o: ../profiler.c ../profiler.h	../profiler_data.h Makefile
	$(SCONECC) -static -DTEST_PROFILER -g -c $< -o $@

libaba.a: aba.o
	ar rcs $@ $<

aba.o: aba.c aba.h ../profiler.h Makefile
	$(SCONECC) -static -DTEST_PROFILER -g -finstrument-functions --include=../profiler.h -c $< -o $@

clean:
	rm test
	rm *.o
