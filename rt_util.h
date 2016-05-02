/**
 * File: rt_util.h
 * Desc: Headers for rt_util.c
 */

#ifndef _H_RT_UTIL_

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sched.h>
#include <sys/mman.h>
#include <string.h>

#define MAX_SAFE_STACK  (8*1024)
#define NSEC_PER_SEC    (1000000000)

typedef struct thread_params {
    int priority;
    int delay;
    long ndelay;
    long interval;
} thread_params;

inline void
schedule(pid_t pid, int policy, const struct sched_param param);

inline void
increment_time(struct timespec *t, long tm);

inline void
copy_time(struct timespec *src, struct timespec *dest);

inline void
normalise_time(struct timespec *t);

inline void
stack_prefault(void);

struct thread_params *
create_thread_params(int priority, int delay, long ndelay, long interval);

#endif
