.PHONY: all clean

HOSTCC ?= $(CC)
SCONECC ?= $(CC)

all: timer libprofiler.a libprofiler.so

timer: timer.c profiler.h profiler_data.h Makefile
	$(HOSTCC) $(CFLAGS) $(EXTRA_FLAGS) -O2 -g -pthread $< -o $@

profiler.o: profiler.c profiler.h profiler_data.h Makefile
	$(SCONECC) $(CFLAGS) $(EXTRA_FLAGS) -O2 -g -fPIC $< -c -o $@

libprofiler.a: profiler.o
	$(AR) rcs $@ $^

libprofiler.so: profiler.o
	$(SCONECC) -shared -o $@ $^ 

clean:
	-rm timer
	-rm libprofiler.so
	-rm libprofiler.a
	-rm profiler.o
