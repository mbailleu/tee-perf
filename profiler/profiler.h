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

#if !defined(__PROFILER_MULTITHREADED)

//Fetches current data pointer and increase global data pointer
static inline struct __profiler_data * const
PERF_METHOD_ATTRIBUTE
__profiler_get_data_ptr(void) {
	assert((uintptr_t)(__profiler_head->data) > (uintptr_t)(__profiler_head));
	assert((uintptr_t)(__profiler_head->data) < ((uintptr_t)__profiler_head) + __profiler_head->size);
	return __profiler_head->data++;
}

static inline void
PERF_METHOD_ATTRIBUTE
__profiler_set_thread(struct __profiler_data * const data) {
    (void) data;
}

#else

#include <pthread.h>

//Fetches current data pointer and increase global data pointer
static inline struct __profiler_data * const
PERF_METHOD_ATTRIBUTE
__profiler_get_data_ptr(void) {
    struct __profiler_data * res;
    asm volatile (
        "lock xadd %[val], %[data]"
        : [val] "=r" (res),
          [data] "+m" (__profiler_head->data)
        : "[val]" (sizeof(struct __profiler_data))
    );
    assert((uintptr_t)res > (uintptr_t)__profiler_head);
    assert((uintptr_t)res < (uintptr_t)__profiler_head + __profiler_head->size);
//    printf("%p\n", res);
    return res;
}

static inline void
PERF_METHOD_ATTRIBUTE
__profiler_set_thread(struct __profiler_data * const data) {
	data->threadID = pthread_self();
}

#endif

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
__profiler_set_version(uint16_t version) {
    __profiler_head->flags = (__profiler_head->flags &(~0xFFFF)) | version;
}

static inline void
PERF_METHOD_ATTRIBUTE
__profiler_set_multithreaded() {
    __profiler_head->flags |= ((uint64_t)1) << 16;
}

static inline void
PERF_METHOD_ATTRIBUTE
__profiler_unset_multithreaded() {
    __profiler_head->flags &= ~(((uint64_t)1) << 16);
}

static inline void
PERF_METHOD_ATTRIBUTE
__profiler_activate_trace() {
    asm volatile (
        "lock bts %[flags], %[dst] \n"
        : [dst] "+m" (__profiler_head->flags)
        : [flags] "i" ((uint8_t)63)
    );
}

static inline void
PERF_METHOD_ATTRIBUTE
__profiler_deactivate_trace() {
    asm volatile (
        "lock btr %[flags], %[dst] \n"
        : [dst] "+m" (__profiler_head->flags)
        : [flags] "i" ((uint8_t)63)
    );
}

static inline int
PERF_METHOD_ATTRIBUTE
__profiler_is_active() {
    uint32_t res;
    asm volatile (
        "lock cmpxchg8b %[ptr] \n"
        : "=a" (res)
        : "a" ((uint64_t)0),
          "b" ((uint64_t)0),
          "c" ((uint64_t)0),
          "d" ((uint64_t)0),
          [ptr] "m" (__profiler_head->flags)
    );
    return res & (1 << 31);
}

static inline void
PERF_METHOD_ATTRIBUTE
__profiler_set_direction(uint64_t * const dir, enum direction_t const val) {
	*dir = (uint64_t)val << 63 | (*dir & (((uint64_t)1 << 63) - 1));
}

static inline void
PERF_METHOD_ATTRIBUTE
__cyg_profile_func(void * const this_fn, enum direction_t const dir) {
	if (__profiler_head == NULL || !__profiler_is_active())
		return;
	struct __profiler_data * const data = __profiler_get_data_ptr();
	__profiler_get_time((__profiler_nsec_t *) &(data->nsec));
	data->callee = this_fn;
	__profiler_set_direction(&(data->direction), dir);
    __profiler_set_thread(data);
}

static void
PERF_METHOD_ATTRIBUTE
__attribute__((unused))
__cyg_profile_func_enter(void * this_fn, void * call_site) {
	__cyg_profile_func(this_fn, CALL);
}

static void
PERF_METHOD_ATTRIBUTE
__attribute__((unused))
__cyg_profile_func_exit(void * this_fn, void * call_site) {
	__cyg_profile_func(this_fn, RET);
}

#if defined(__cplusplus)
}
#endif //__cplusplus
