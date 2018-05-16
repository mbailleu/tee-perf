#pragma once

#include <stddef.h>
#include <stdint.h>

#if !defined(PERF_ENV_SHM_VAR)
#define PERF_ENV_SHM_VAR "SGXPROFILERSHM"
#endif

typedef uint64_t __profiler_sec_t;
typedef uint64_t __profiler_nsec_t;
typedef uint64_t __profiler_pid_t;

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
 */
struct __profiler_data {
	union {
		uint64_t direction;
		__profiler_nsec_t nsec;
	};
	void * callee;
} __attribute__((packed));

struct __profiler_header {
	__profiler_nsec_t volatile nsec;
	struct __profiler_header * self;
	__profiler_pid_t  volatile scone_pid;
	size_t size;
	struct __profiler_data * data;
} __attribute__((packed,aligned(8)));
