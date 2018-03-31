#pragma once

#include <assert.h>

#include "profiler_data.h"

#define PERF_METHOD_ATTRIBUTE \
	__attribute__((no_instrument_function,hot))


extern struct __profiler_header * __profiler_head;

static inline struct __profiler_data *
PERF_METHOD_ATTRIBUTE
__profiler_fetch_data_ptr(void) {
#if defined(TEST_PROFILER)
	assert((intptr_t)(__profiler_head->data) > (intptr_t)(__profiler_head));
	assert((intptr_t)(__profiler_head->data) < ((intptr_t)__profiler_head) + __profiler_head->size);
#endif
	return __profiler_head->data++;
}

static inline void
PERF_METHOD_ATTRIBUTE
__profiler_get_time(__profiler_sec_t * sec, __profiler_nsec_t * nsec) {
	asm volatile (
		"lock cmpxchg16b %[ptr] \n"
		: "=a" (*sec),
		  "=d" (*nsec)
		: "a" ((uint64_t)0),
		  "b" ((uint64_t)0),
		  "c" ((uint64_t)0),
		  "d" ((uint64_t)0),
		  [ptr] "m" (__profiler_head->sec)
	);
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

static void PERF_METHOD_ATTRIBUTE __cyg_profile_func_enter(void * this_fn, void * call_site) {
	__cyg_profile_func(this_fn, call_site, CALL);
}

static void PERF_METHOD_ATTRIBUTE __cyg_profile_func_exit(void * this_fn, void * call_site) {
	__cyg_profile_func(this_fn, call_site, RET);
}

