.PHONY: clean

timer: timer.c profiler_data.h Makefile
	$(CC) $(CFLAGS) $(EXTRA_FLAGS) -O2 -g -pthread $< -o $@

clean:
	rm timer
