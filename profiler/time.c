#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>

#include "profiler_data.h"

#define PROG_NAME "timer"

#define prog_fprint(fd, str, ...) \
	do { \
		fprintf(fd, PROG_NAME ": " str, ##__VA_ARGS__); \
	} while(0)

#define print(...) \
	prog_fprint(stdout, __VA_ARGS__)

#define print_error(...) \
	prog_fprint(stderr, __VA_ARGS__)

static struct __profiler_header * head = NULL;
static int shm_fd = -1;

static int create_shared_memory(char const * shm_name, size_t const mem_size) {
	int fd = open(shm_name, O_RDWR | O_CREAT | O_CLOEXEC, (mode_t)0600);
	if (fd == -1) {
		print_error("Could not open %s\n", shm_name);
		return 1;
	}
	size_t sz = lseek(fd, mem_size, SEEK_SET);
	if (sz == -1) {
		close(fd);
		print_error("Could not resize %s to %lu\n", shm_name, mem_size);
		return 2;
	}

	size_t written = write(fd, "", 1);
	if (written == -1) {
		close(fd);
		print_error("Could not write to file %s\n", shm_name);
		return 4;
	}
	
	void * ptr = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (ptr == MAP_FAILED) {
		close(fd);
		print_error("Could not map %s\n", shm_name);
		return 3;
	}

	shm_fd = fd;
	head = (struct __profiler_header *)ptr;
	head->size = sz;
	return 0;
}

static void unmap_shared_memory() {
	if (head != NULL) {
		if (munmap(head, head->size)) {
			print_error("Could not unmap %p\n", head);
		}
		head = NULL;
	}
	if (shm_fd != -1) {
		close(shm_fd);
		shm_fd = -1;
	}
}

static pid_t start_other(char * program, char ** args) {
	pid_t pid;
	if ((pid = fork()) == 0) {
		execv(program, args);
		exit(127);
	}
	return pid;
}

static __attribute__((hot)) void * update_clock(void * ptr) {
	struct timespec * t = (struct timespec *)&(head->sec);
	for(;;) {
		clock_gettime(CLOCK_MONOTONIC, t);
//		asm volatile (
//			""
//			:
//			: [sec] "m" (head->sec),
//		  	  [nsec] "m" (head->nsec)
//		);
	}
}

int main(int argc, char ** argv) {
	if (argc < 3) {
		printf("%s needs 2 arguments, filename and different program\n", argv[0]);
		return 1;
	}
	char const * shm_name = argv[1];

	int ret;
	if ((ret = create_shared_memory(shm_name, __PROFILER_SHM_SIZE__))) {
		return ret;
	}

	pid_t child = start_other(argv[2], argv + 3);
 	pthread_t clock;
	if ((ret = pthread_create(&clock, NULL, update_clock, NULL)) != 0) {
		print_error("Could not create clock thread\n");
		return ret;
	}

	waitpid(child, &ret, 0);
	if (ret != 0) {
		print_error("Child exited with code: %d\n", ret);
	}
	return 0;
//	pthread_cancel(clock);
//	pthread_join(clock, NULL);
//	unmap_shared_memory();
}
