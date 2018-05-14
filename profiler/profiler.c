#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#if defined(TEST_PROFILER)
#include <assert.h>
#endif

#include "profiler.h"
#include "profiler_data.h"

#if defined(FILENAME)
#define __PROFILER_OLD_FILENAME FILENAME
#undef FILENAME
#endif

#if defined(CONSTRUCTOR)
#define __PROFILER_OLD_CONSTRUCTOR CONSTRUCTOR
#undef CONSTRUCTOR
#endif

//#define FILENAME "__profiler_file_scone.shm"
#define FILENAME "/tmp/__profiler_file_scone.shm"
#define CONSTRUCTOR(x) \
	static void \
    __attribute__((constructor,used,no_instrument_function,cold)) \
    x ## _constructor(void) { \
		x();                              \
	}

#define DESTRUCTOR(x) \
	static void \
    __attribute__((destructor,used,no_instrument_function,cold)) \
    x ## _destructor(void) { \
		x();                              \
	}

#define mem_size __PROFILER_SHM_SIZE__
#define COULD_NOT_GET_SHM "could not find " PERF_ENV_SHM_VAR " enviroment variable"
#define OPEN_ERROR "could not open " FILENAME "\n"
#define SIZE_ERROR "could not resize " FILENAME " to 16KiB\n" 
#define MAP_ERROR  "__profiler_head could not be intanziated\n"
#define UMAP_ERROR "could not unmap memory\n"
#define SEEK_ERROR "could not seek to position\n"
#define READ_ERROR "could not read\n"

struct __profiler_header * __profiler_head = NULL;

static int __profiler_fd = -1;
static size_t __profiler_map_size = 0;

static void __attribute__((no_instrument_function,cold)) __profiler_map_info(void) {
	char * envv = getenv(PERF_ENV_SHM_VAR);
	if (envv == NULL) {
		write(2, COULD_NOT_GET_SHM, sizeof(COULD_NOT_GET_SHM));
		return;
	}

	int fd = 0;
	for (size_t i = 0; envv[i] != 0; ++i) {
		fd = fd * 10 + (envv[i] - '0');
	}

	if (fd == -1) {
		write(2, OPEN_ERROR, sizeof(OPEN_ERROR));
		return;
	}

#if 0
	size_t sz = lseek(fd, mem_size, SEEK_SET);
	if (sz == -1) {
		close(fd);
		write(2, SIZE_ERROR, sizeof(SIZE_ERROR));
		return;
	}

	size_t written = write(fd, "", 1);
	if (written == -1) {
		close(fd);
		write(2,SIZE_ERROR, sizeof(SIZE_ERROR));
		return;
	}
#endif
	size_t sz = lseek(fd, offsetof(struct __profiler_header, size), SEEK_SET);
	if (sz == -1) {
		close(fd);
		write(2, SEEK_ERROR, sizeof(SEEK_ERROR));
		return;
	}

	if (read(fd, &sz, sizeof(size_t)) == -1) {
		close(fd);
		write(2, READ_ERROR, sizeof(READ_ERROR));
		return;
	}

	void * ptr = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); 
	if (ptr == MAP_FAILED) {
		close(fd);
		write(2, MAP_ERROR, sizeof(MAP_ERROR));
		return;
	}
	__profiler_fd = fd;
	__profiler_head = (struct __profiler_header *)ptr;
	__profiler_head->self = __profiler_head;
	__profiler_map_size = sz;
	__profiler_head->scone_pid = getpid();
	__profiler_head->data = (struct __profiler_data *)(__profiler_head + 1);
	//busy Wait until timer works
	__profiler_nsec_t nsec;
	do {
		__profiler_get_time(&nsec);
	} while (nsec == 0);
}

static void __attribute__((no_instrument_function,cold)) __profiler_unmap_info(void) {
	if (__profiler_head != NULL) {
		void * ptr = (void *)__profiler_head;
		size_t const sz = __profiler_map_size;
		__profiler_head = NULL;
		__profiler_map_size = 0;
		if (munmap(ptr, sz)) {
			write(2, UMAP_ERROR, sizeof(UMAP_ERROR));
		}
	}
	int fd = __profiler_fd;
	__profiler_fd = -1;
	if (fd != -1)
		close(fd);
}

CONSTRUCTOR(__profiler_map_info);
DESTRUCTOR(__profiler_unmap_info);

#undef FILENAME
#if defined(__PROFILER_OLD_FILENAME)
#define FILENAME __PROFILER_OLD_FILENAME
#undef __PROFILER_OLD_FILENAME
#endif

#undef CONSTRUCTOR
#if defined(__PROFILER_OLD_CONSTRUCTOR)
#define CONSTRUCTOR __PROFILER_OLD_CONSTRUCTOR
#undef __PROFILER_OLD_CONSTRUCTOR
#endif


//#include <stdint.h>
//#include <stddef.h>

