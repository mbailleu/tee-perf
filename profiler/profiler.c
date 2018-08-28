#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stddef.h>
#include <stdlib.h>

#include "profiler.h"
#include "profiler_data.h"

#if defined(CONSTRUCTOR)
#define __PROFILER_OLD_CONSTRUCTOR CONSTRUCTOR
#undef CONSTRUCTOR
#endif

#if defined(DESTRUCTOR)
#define __PROFILER_OLD_DESTRUCTOR DESTRUCTOR
#undef DESTRUCTOR
#endif

#define CONSTRUCTOR(x) \
	static void \
    __attribute__((constructor,used,no_instrument_function,cold)) \
    x ## _constructor(void) { \
		x(); \
	}

#define DESTRUCTOR(x) \
	static void \
    __attribute__((destructor,used,no_instrument_function,cold)) \
    x ## _destructor(void) { \
		x(); \
	}

#define COULD_NOT_GET_SHM "could not find " PERF_ENV_SHM_VAR " enviroment variable\n"
#define OPEN_ERROR "could not open shared memory area\n"
#define MAP_ERROR  "__profiler_head could not be intanziated\n"
#define UMAP_ERROR "could not unmap memory\n"
#define SEEK_ERROR "could not seek to position\n"
#define READ_ERROR "could not read\n"

struct __profiler_header * __profiler_head = NULL;

static int __profiler_fd = -1;
static size_t __profiler_map_size = 0;

#if defined(__cplusplus)
extern "C" {
#endif //__cplusplus

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

	if (fd < 0) {
		write(2, OPEN_ERROR, sizeof(OPEN_ERROR));
		return;
	}

	size_t sz = lseek(fd, offsetof(struct __profiler_header, size), SEEK_SET);
	if (sz == ~0UL) {
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
	__profiler_head->flags = 0;
	__profiler_set_version(1);
#if defined(__PROFILER_MULTITHREADED)
    __profiler_set_multithreaded();
#else
    __profiler_unset_multithreaded();
#endif
    __profiler_deactivate_trace();
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

CONSTRUCTOR(__profiler_map_info)
DESTRUCTOR(__profiler_unmap_info)

#if defined(__cplusplus)
} //extern "C"
#endif //__cplusplus

#undef CONSTRUCTOR
#if defined(__PROFILER_OLD_CONSTRUCTOR)
#define CONSTRUCTOR __PROFILER_OLD_CONSTRUCTOR
#undef __PROFILER_OLD_CONSTRUCTOR
#endif

#undef DESTRUCTOR
#if defined(__PROFILER_OLD_DESTRUCTOR)
#define DESTRUCTOR __PROFILER_OLD_DESTRUCTOR
#undef __PROFILER_OLD_DESTRUCTOR
#endif

//#include <stdint.h>
//#include <stddef.h>

