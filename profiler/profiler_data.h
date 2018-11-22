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
	RET  = 1,
	ACTIVATE = 1,
	DEACTIVATE = 0
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
 *
 *  Two special entries exists:
 *  Activating events, and deactivating events
 *  The direction bit is used to denote activation or deactivation
 *  nsec still remains the same, callee and if availlable threadID will be set to 0
 *  
 *  
 *  2 +-+----------------+
 *    |A|     nsec       |
 *  1 +-+----------------+
 *    |       0x0        |
 *  0 +------------------+
 *   64 63               0
 *
 *   A     - Activation if 1, Deactivation if 0
 *   nsec  - Relative time since start of timer app
 *
 *
 * Multithreaded:
 * 3 +-+-----------------+
 *   |A|     nsec        |
 * 2 +-+-----------------+
 *   |       0x0         |
 * 1 +-+-----------------+
 *   |       0x0         |
 * 0 +-+-----------------+
 *  64 63                0
 *
 *  A     - Activating if 1, Deactivation if 0
 *  nsec  - Relative time since start of timer app
 *
 */
struct __profiler_data {
	union {
		__profiler_direction_t direction;
		__profiler_nsec_t nsec;
	};
	void * 		callee;
#if defined(__PROFILER_MULTITHREADED)
	uint64_t 	threadID;
#endif
} __attribute__((packed));

struct __profiler_header {
	__profiler_nsec_t volatile nsec;
/*
 *  +-+---------+-+-------+
 *  |A|  unused |M|Version|
 *  +-+---------+-+-------+
 * 64 63       16 15      0
 *
 * A       - If currently the application gets profiled
 * M       - If the application is profiled with multihread support
 * Version - Version number of the currently used log format
 */
	uint64_t flags;
	struct __profiler_header * self;
	__profiler_pid_t  volatile scone_pid;
	size_t size;
	uint64_t idx;
	uintptr_t __profiler_mem_location;
} __attribute__((packed,aligned(8)));
