.PHONY: all clean

SRCDIR=../..
SCONECC ?= $(CC)
HOSTCC ?= $(CC)

all: test timer

timer: $(SRCDIR)/timer.c $(SRCDIR)/profiler_data.h Makefile
	$(HOSTCC) -g -pthread $< -o $@ -Wall -Wpedantic

test: main.o profiler.o
	$(SCONECC) -static -DTEST_PROFILER -D__PROFILER_MULTITHREADED -g -finstrument-functions main.o profiler.o -o $@ -pthread -Wall -Wpedantic

main.o: main.c $(SRCDIR)/profiler.h $(SRCDIR)/profiler_data.h Makefile
	$(SCONECC) -static -DTEST_PROFILER -D__PROFILER_MULTITHREADED -g -finstrument-functions --include=$(SRCDIR)/profiler.h -c $< -o $@ -Wall -Wpedantic -Werror

profiler.o: $(SRCDIR)/profiler.c $(SRCDIR)/profiler.h $(SRCDIR)/profiler_data.h Makefile
	$(SCONECC) -static -DTEST_PROFILER -D__PROFILER_MULTITHREADED -g -c $< -o $@ -Wall -Wpedantic -Werror

clean:
	rm test
	rm timer
	rm *.o
