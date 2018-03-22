#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "profiler_data.h"
static struct __profiler_header * head = NULL;
static int shm_fd = -1;

static int create_shared_memory(char const * shm_name, size_t const mem_size) {
	int fd = open(shm_name, O_RDWR | O_CREAT | O_EXC, (mode_t)0600);
	if (fd == -1) {
		fprintf(stderr, "Could not open %s\n", shm_name);
		return 1;
	}

	size_t sz = lseek(fd, mem_size, SEEK_SET);
	if (sz == -1) {
		close(fd);
		fprintf(stderr, "Could not resize %s to %lu\n", shm_name, mem_size);
		return 2;
	}
	
	void * ptr = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (ptr == MAP_FAILED) {
		close(fd);
		fprintf(stderr, "Could not map %s\n", shm_name);
		return 3;
	}

	shm_fd = fd;
	head = (struct __profiler_head *)ptr;
	head->size = sz;
	head->data = (struct __profiler_data *)(head + 1);
	return 0;
}

static void unmap_shared_memory() {
	if (head != NULL) {
		if (munmap(head, head->size)) {
			fprintf(stderr, "Could not unmap %p\n", head);
		}
		head = NULL;
	}
	if (shm_fd != -1)
		close(fd);
}

static pid_t start_other(char * program, char ** args) {
	pid_t pid;
	if ((pid = fork()) == 0) {
		execv(program, args);
		exit(127);
	}
	return pid;
}

static void * update_clock(void * ptr) {
}

int main(int argc, char ** argv) {
	if (argc < 3) {
		printf("%s needs 2 argument, filename and different program\n", argv[0]);
		return 1;
	}
	char const * shm_name = argv[1];

	int ret;
	if ((ret = create_shared_memory(shm_name, __PROFILER_SHM_SIZE__))) {
		return ret;
	}

	pid_t pid = start_other(argv[2], argv + 3);
}
