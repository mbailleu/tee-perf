#pragma once

#if defined(__cplusplus)
extern "C" {
#endif //__cplusplus

#include <assert.h>

#include "profiler_data.h"

//Our methods get called often, also we do not want to trace ourself
#define PERF_METHOD_ATTRIBUTE \
	__attribute__((no_instrument_function,hot))

//Instance in profiler.c
extern struct __profiler_header * __profiler_head;

//Fetches current data pointer and increase global data pointer
//TODO Multithread safe
static inline struct __profiler_data * const
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
__profiler_get_time(__profiler_nsec_t * const nsec) {
	struct T {
		uint32_t lower;
		uint32_t higher;
	} * const tmp = (struct T * const) nsec;

	asm volatile (
		"lock cmpxchg8b %[ptr] \n"
		: "=a" (tmp->lower),
		  "=d" (tmp->higher)
		: "a" ((uint64_t)0),
		  "b" ((uint64_t)0),
		  "c" ((uint64_t)0),
		  "d" ((uint64_t)0),
		  [ptr] "m" (__profiler_head->nsec)
	);
}

static inline void
PERF_METHOD_ATTRIBUTE
__profiler_set_direction(uint64_t * const dir, enum direction_t const val) {
	*dir = (uint64_t)val << 63 | (*dir & (((uint64_t)1 << 63) - 1));
}

static inline void
PERF_METHOD_ATTRIBUTE
__cyg_profile_func(void * const this_fn, enum direction_t const dir) {
	if (__profiler_head == NULL)
		return;
	struct __profiler_data * const data = __profiler_fetch_data_ptr();
	__profiler_get_time((__profiler_nsec_t *) &(data->nsec));
	data->callee = this_fn;
	__profiler_set_direction(&(data->direction), dir);
}

static void PERF_METHOD_ATTRIBUTE __cyg_profile_func_enter(void * this_fn, void * call_site) {
	__cyg_profile_func(this_fn, CALL);
}

static void PERF_METHOD_ATTRIBUTE __cyg_profile_func_exit(void * this_fn, void * call_site) {
	__cyg_profile_func(this_fn, RET);
}

#if defined(__cplusplus)
}
#endif //__cplusplus
