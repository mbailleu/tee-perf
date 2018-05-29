#pragma once

#include <stddef.h>
#include <stdint.h>

#if defined(__PROFILER_MULTITHREADED)
#include <pthread.h>
#endif

#if !defined(PERF_ENV_SHM_VAR)
#define PERF_ENV_SHM_VAR "SGXPROFILERSHM"
#endif

typedef uint64_t __profiler_sec_t;
typedef uint64_t __profiler_nsec_t;
typedef uint64_t __profiler_pid_t;
typedef __profiler_nsec_t __profiler_direction_t;

enum direction_t {
	CALL = 0,
	RET  = 1
};

/*
 * 2 +-+-----------------+
 *   |D|     nsec        |
 * 1 +-+-----------------+
 *   |      callee       |
 * 0 +-+-----------------+
 *  64 63                0
 *
 *  D      - Direction see direction_t
 *  nsec   - Relative time since start of timer app
 *  callee - Callee address of method
 *
 * Multithreaded:
 * 3 +-+-----------------+
 *   |D|     nsec        |
 * 2 +-+-----------------+
 *   |      callee       |
 * 1 +-+-----------------+
 *   |     threadID      |
 * 0 +-+-----------------+
 *  64 63                0
 *
 *  D      - Direction see direction_t
 *  nsec   - Relative time since start of timer app
 *  callee - Callee address of method
 */
struct __profiler_data {
	union {
		__profiler_direction_t direction;
		__profiler_nsec_t nsec;
	};
	void * 		callee;
#if defined(__PROFILER_MULTITHREADED)
	pthread_t 	threadID;
#endif
} __attribute__((packed));

struct __profiler_header {
	__profiler_nsec_t volatile nsec;
	struct __profiler_header * self;
	__profiler_pid_t  volatile scone_pid;
	size_t size;
	struct __profiler_data * data;
} __attribute__((packed,aligned(8)));
