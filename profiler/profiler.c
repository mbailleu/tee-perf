#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#include "profiler.h"

#define PERF_METHOD_ATTRIBUTE \
	__attribute__((no_instrument_function))

typedef uint64_t __profiler_sec_t;
typedef uint64_t __profiler_nsec_t;
typedef int      __profiler_pid_t;

enum direction_t {
	CALL = 0,
	RET  = 1
};

struct __profiler_data {
	__profiler_sec_t  sec;
	__profiler_nsec_t nsec;
	void * callee;
	void * caller;
	enum direction_t direction;
};

struct __profiler_header {
	struct __profiler_header * self;
	__profiler_sec_t  volatile sec;
	__profiler_nsec_t volatile nsec;
	__profiler_pid_t  volatile scone_pid;
	size_t size;
	struct __profiler_data * data;
};

struct __profiler_header * __profiler_head = NULL;

static inline struct __profiler_data *
PERF_METHOD_ATTRIBUTE
__profiler_fetch_data_ptr(void) {
	return __profiler_head->data++;
}

static inline void
PERF_METHOD_ATTRIBUTE
__profiler_get_time(__profiler_sec_t * sec, __profiler_nsec_t * nsec) {
	*sec = __profiler_head->sec;
	*nsec = __profiler_head->nsec;
}

static inline void
PERF_METHOD_ATTRIBUTE
__cyg_profile_func(void * const this_fn, void * const call_site, enum direction_t const dir) {
	if (__profiler_head == NULL)
		return;
	struct __profiler_data * data = __profiler_fetch_data_ptr();
	__profiler_get_time(&(data->sec), &(data->nsec));
	data->callee = this_fn;
	data->caller = call_site;
	data->direction = dir;
}

void
PERF_METHOD_ATTRIBUTE
__cyg_profile_func_enter(void * this_fn, void * call_site) {
	__cyg_profile_func(this_fn, call_site, CALL);
}

void
PERF_METHOD_ATTRIBUTE
__cyg_profile_func_exit(void * this_fn, void * call_site) {
	__cyg_profile_func(this_fn, call_site, RET);
}

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
    __attribute__((constructor,used,no_instrument_function)) \
    x ## _constructor(void) { \
		x();                              \
	};

#define mem_size (4 << 12)
#define OPEN_ERROR "could not open " FILENAME "\n"
#define SIZE_ERROR "could not resize " FILENAME " to 16KiB\n" 
#define MAP_ERROR  "__profiler_head could not be intanziated\n"
#define UMAP_ERROR "could not unmap memory\n"

static int __profiler_fd = -1;
static size_t __profiler_map_size = 0;

static void PERF_METHOD_ATTRIBUTE __profiler_map_info(void) {
	//TODO: This is only a dummy implementation
	int fd = open(FILENAME, O_RDWR | O_CREAT, (mode_t)0600);
	if (fd == -1) {
		write(2, OPEN_ERROR, sizeof(OPEN_ERROR));
		return;
	}

	size_t sz = lseek(fd, mem_size, SEEK_SET);
	if (sz == -1) {
		close(fd);
		write(2, SIZE_ERROR, sizeof(SIZE_ERROR));
		return;
	}

	sz = write(fd, "", 1);
	if (sz == -1) {
		close(fd);
		write(2,SIZE_ERROR, sizeof(SIZE_ERROR));
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
	__profiler_map_size = sz;
	__profiler_head->self = __profiler_head;
	__profiler_head->size = sz;
	__profiler_head->data = (struct __profiler_data *)(__profiler_head + 1);
}

CONSTRUCTOR(__profiler_map_info);

#if defined(TEST_PROFILER)

static void __attribute__((destructor,used,no_instrument_function)) dump(void) {
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

#endif

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


#include <stdint.h>
#include <stddef.h>

