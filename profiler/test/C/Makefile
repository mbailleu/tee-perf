.PHONY: all clean

SRCDIR=../..
SCONECC ?= $(CC)
HOSTCC ?= $(CC)

all: test timer

timer: $(SRCDIR)/timer.c $(SRCDIR)/profiler_data.h Makefile
	$(HOSTCC) -g -lpthread $< -o $@

test: main.o profiler.o libaba.a
	$(SCONECC) -static -DTEST_PROFILER -g -finstrument-functions main.o profiler.o -o $@ -L./ -laba -Wall -Wpedantic -Werror

main.o: main.c $(SRCDIR)/profiler.h $(SRCDIR)/profiler_data.h aba.h Makefile
	$(SCONECC) -static -DTEST_PROFILER -g -finstrument-functions --include=$(SRCDIR)/profiler.h -c $< -o $@ -Wall -Wpedantic -Werror

profiler.o: $(SRCDIR)/profiler.c $(SRCDIR)/profiler.h $(SRCDIR)/profiler_data.h Makefile
	$(SCONECC) -static -DTEST_PROFILER -g -c $< -o $@ -Wall -Wpedantic

libaba.a: aba.o
	ar rcs $@ $<

aba.o: aba.c aba.h $(SRCDIR)/profiler.h Makefile
	$(SCONECC) -static -DTEST_PROFILER -g -finstrument-functions --include=$(SRCDIR)/profiler.h -c $< -o $@ -Wall -Wpedantic -Werror

clean:
	rm test
	rm timer
	rm *.o
	rm *.a
