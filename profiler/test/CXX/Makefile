.PHONY: all clean

SRCDIR=../..
SCONECXX ?= $(CXX)
HOSTCC ?= $(CC)

all: test timer

timer: $(SRCDIR)/timer.c $(SRCDIR)/profiler_data.h Makefile
	$(HOSTCC) -g -lpthread $< -o $@

test: main.o profiler.o
	$(SCONECXX) -static -DTEST_PROFILER -D__PROFILER_MULTITHREADED -g -finstrument-functions main.o profiler.o -o $@

main.o: main.cpp $(SRCDIR)/profiler.h $(SRCDIR)/profiler_data.h Makefile
	$(SCONECXX) -static -DTEST_PROFILER -D__PROFILER_MULTITHREADED -g -finstrument-functions --include=$(SRCDIR)/profiler.h -c $< -o $@

profiler.o: $(SRCDIR)/profiler.c $(SRCDIR)/profiler.h $(SRCDIR)/profiler_data.h Makefile
	$(SCONECXX) -static -DTEST_PROFILER -D__PROFILER_MULTITHREADED -g -c $< -o $@

clean:
	-rm test
	-rm timer
	-rm *.o
