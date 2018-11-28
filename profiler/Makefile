.PHONY: clean

all: timer libprofiler.a libprofiler.so

timer: timer.c profiler.h profiler_data.h Makefile
	$(CC) $(CFLAGS) $(EXTRA_FLAGS) -O2 -g -pthread $< -o $@

profiler.o: profiler.c profiler.h profiler_data.h Makefile
	$(CC) $(CFLAGS) $(EXTRA_FLAGS) -O2 -g -fPIC $< -c -o $@

libprofiler.a: profiler.o
	$(AR) rcs $@ $^

libprofiler.so: profiler.o
	$(CC) -shared -o $@ $^ 

clean:
	-rm timer
	-rm libprofiler.so
	-rm libprofiler.a
	-rm profiler.o
