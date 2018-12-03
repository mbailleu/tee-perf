#pragma once

#if !defined(__PROFILER__)
#define __PROFILER__
#endif

#if defined(__cplusplus)
extern "C" {
#endif //__cplusplus

#include <assert.h>

#include "profiler_data.h"

#if !defined(PERF_METHOD_ATTRIBUTE)
//Our methods get called often, also we do not want to trace ourself
#define PERF_METHOD_ATTRIBUTE \
	__attribute__((no_instrument_function,hot))
#endif

#if !defined(perf_log_head)
#define perf_log_head __profiler_head
#endif

//Instance in profiler.c
extern struct __profiler_header * perf_log_head;

#if defined(PROFILER_WARP_AROUND)
extern uint64_t __profiler_mask;

static inline uint64_t __profiler_warp_around(uint64_t num) {
    return num & __profiler_mask;
}

#else

static inline uint64_t __profiler_warp_around(uint64_t num) {
    return num;
}

#endif

#if !defined(__PROFILER_MULTITHREADED)

//Fetches current data pointer and increase global data pointer
static inline struct __profiler_data * const
PERF_METHOD_ATTRIBUTE
__profiler_get_data_ptr(void) {
    struct __profiler_data * res = ((struct __profiler_data *)(perf_log_head + 1) + (__profiler_warp_around(perf_log_head->idx++)));
	assert((uintptr_t)(res) > (uintptr_t)(perf_log_head));
	assert((uintptr_t)(res) < ((uintptr_t)perf_log_head) + perf_log_head->size);
	return res;
}

static inline void
PERF_METHOD_ATTRIBUTE
__profiler_set_thread(struct __profiler_data * const data, uint64_t const threadID) {
    (void) data;
    (void) threadID;
}

static inline uint64_t
PERF_METHOD_ATTRIBUTE
__profiler_get_thread_id() {
    return 0;
}

#else

#include <pthread.h>

//Fetches current data pointer and increase global data pointer
static inline struct __profiler_data * const
PERF_METHOD_ATTRIBUTE
__profiler_get_data_ptr(void) {
    uint64_t idx;
    asm volatile (
        "lock xadd %[val], %[data]"
        : [val] "=r" (idx),
          [data] "+m" (perf_log_head->idx)
        : "[val]" (1)
    );
    struct __profiler_data * res = ((struct __profiler_data *)(perf_log_head + 1)) + __profiler_warp_around(idx);
    assert((uintptr_t)res > (uintptr_t)perf_log_head);
    assert((uintptr_t)res < (uintptr_t)perf_log_head + perf_log_head->size);
//    printf("%p\n", res);
    return res;
}

static inline void
PERF_METHOD_ATTRIBUTE
__profiler_set_thread(struct __profiler_data * const data, uint64_t const threadID) {
	data->threadID = threadID;
}

static inline uint64_t
PERF_METHOD_ATTRIBUTE
__profiler_get_thread_id() {
    return (uint64_t)pthread_self();
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
		  [ptr] "m" (perf_log_head->nsec)
	);
}

static inline void
PERF_METHOD_ATTRIBUTE
__profiler_set_version(uint16_t version) {
    perf_log_head->flags = (perf_log_head->flags &(~0xFFFF)) | version;
}

static inline void
PERF_METHOD_ATTRIBUTE
__profiler_set_multithreaded() {
    perf_log_head->flags |= ((uint64_t)1) << 16;
}

static inline void
PERF_METHOD_ATTRIBUTE
__profiler_unset_multithreaded() {
    perf_log_head->flags &= ~(((uint64_t)1) << 16);
}

static inline void
PERF_METHOD_ATTRIBUTE
__profiler_set_direction(uint64_t * const dir, enum direction_t const val) {
	*dir = (uint64_t)val << 63 | (*dir & (((uint64_t)1 << 63) - 1));
}

static inline void
PERF_METHOD_ATTRIBUTE
__profiler_write_entry(void * const this_fn, enum direction_t const val, uint64_t const threadID) {
    struct __profiler_data * const data = __profiler_get_data_ptr();
    __profiler_get_time((__profiler_nsec_t *) &(data->nsec));
    __profiler_set_direction(&(data->direction), val);
    data->callee = this_fn;
    __profiler_set_thread(data, threadID);
}

static inline void
PERF_METHOD_ATTRIBUTE
__profiler_write_activation_record(enum direction_t const val) {
    __profiler_write_entry(NULL, val, 0);
}

static inline void
PERF_METHOD_ATTRIBUTE
__profiler_activate_trace() {
    if (perf_log_head == NULL)
        return;
    asm volatile (
        "lock bts %[flags], %[dst] \n"
        : [dst] "+m" (perf_log_head->flags)
        : [flags] "i" ((uint8_t)63)
    );
}

static inline void
PERF_METHOD_ATTRIBUTE
__profiler_activate() {
    if (perf_log_head == NULL)
        return;
    __profiler_activate_trace();
    __profiler_write_activation_record(ACTIVATE);
}


static inline void
PERF_METHOD_ATTRIBUTE
__profiler_deactivate_trace() {
    if (perf_log_head == NULL)
        return;
    asm volatile (
        "lock btr %[flags], %[dst] \n"
        : [dst] "+m" (perf_log_head->flags)
        : [flags] "i" ((uint8_t)63)
    );
}

static inline void
PERF_METHOD_ATTRIBUTE
__profiler_deactivate() {
    if (perf_log_head == NULL)
        return;
    __profiler_deactivate_trace();
    __profiler_write_activation_record(DEACTIVATE);
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
          [ptr] "m" (perf_log_head->flags)
    );
    return res & (1 << 31);
}

static inline void
PERF_METHOD_ATTRIBUTE
__cyg_profile_func(void * const this_fn, enum direction_t const dir) {
	if (perf_log_head == NULL || !__profiler_is_active())
		return;
	__profiler_write_entry(this_fn, dir, __profiler_get_thread_id());
}

static void
PERF_METHOD_ATTRIBUTE
__attribute__((used))
__cyg_profile_func_enter(void * this_fn, void * call_site) {
	__cyg_profile_func(this_fn, CALL);
}

static void
PERF_METHOD_ATTRIBUTE
__attribute__((used))
__cyg_profile_func_exit(void * this_fn, void * call_site) {
	__cyg_profile_func(this_fn, RET);
}

#if defined(__cplusplus)
}
#endif //__cplusplus
