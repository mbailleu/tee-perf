#pragma once

#define __PROFILER_SHM_KEY__ 4242
#define __PROFILER_SHM_SIZE__ (1 << 12)

typedef uint64_t __profiler_sec_t;typedef uint64_t __profiler_nsec_t;
typedef uint64_t __profiler_pid_t;

enum direction_t {
	CALL = 0,
	RET  = 1
};

struct __profiler_data {
	__profiler_sec_t  sec;
	__profiler_nsec_t nsec;
	void * callee;
	void * caller;
	uint64_t direction;
} __attribute__((packed));

struct __profiler_header {
	struct __profiler_header * self;
	__profiler_sec_t  volatile sec;
	__profiler_nsec_t volatile nsec;
	__profiler_pid_t  volatile scone_pid;
	size_t size;
	struct __profiler_data * data;
} __attribute__((packed));
