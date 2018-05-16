.PHONY: all clean

SRCDIR=../..
SCONECC ?= $(CXX)
HOSTCC ?= $(CXX)

all: test timer

timer: $(SRCDIR)/timer.c $(SRCDIR)/profiler_data.h Makefile
	$(HOSTCC) -g -lpthread $< -o $@

test: main.o profiler.o
	$(SCONECC) -static -DTEST_PROFILER -g -finstrument-functions main.o profiler.o -o $@ -L./ -laba

main.o: main.c $(SRCDIR)/profiler.h $(SRCDIR)/profiler_data.h Makefile
	$(SCONECC) -static -DTEST_PROFILER -g -finstrument-functions --include=$(SRCDIR)/profiler.h -c $< -o $@

profiler.o: $(SRCDIR)/profiler.c $(SRCDIR)/profiler.h $(SRCDIR)/profiler_data.h Makefile
	$(SCONECC) -static -DTEST_PROFILER -g -c $< -o $@

clean:
	-rm test
	-rm timer
	-rm *.o
