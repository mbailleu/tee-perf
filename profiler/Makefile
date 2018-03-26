.PHONY: clean

timer: timer.c profiler_data.h Makefile
	$(CC) $(CFLAGS) $(EXTRA_FLAGS) -g -lpthread $< -o $@

clean:
	rm timer
