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
#include <pthread.h>

#define MAX_SAFE_STACK  (8*1024)
#define NSEC_PER_SEC    (1000000000)

#define INVALID_TIME 0
#define INVALID_THREAD 0x0

typedef enum JOB_STATE
{
    JOB_STATE_UNKOWN,
    JOB_STATE_DONE,
    JOB_STATE_BUSY,
    JOB_STATE_ERROR
} JOB_STATE;

typedef struct last_thread {
    pthread_mutex_t mutex;
    unsigned long start_time;
    unsigned long end_time;
    unsigned char thread_id;
    JOB_STATE state;
} last_thread;

typedef struct thread_params {
    unsigned int priority;
    unsigned int delay;
    unsigned long ndelay;
    unsigned long interval;
    unsigned char thread_id;
    last_thread *lt;
} thread_params;

inline void
schedule(pid_t pid, int policy, const struct sched_param param);

inline void
increment_time(struct timespec *t, unsigned long tm);

inline void
increment_time_u(struct timespec *t, unsigned long tm);

inline void
copy_time(struct timespec *src, struct timespec *dest);

inline void
normalise_time(struct timespec *t);

inline void
stack_prefault(void);

struct thread_params *
create_thread_params(unsigned int priority, unsigned int delay,
                     unsigned long ndelay, unsigned long interval,
                     unsigned char thread_id, last_thread *lt);

struct last_thread *
create_last_thread();

inline void
dump_last_thread_data(unsigned int thread_id, char *message, last_thread *lt);

inline void
begin_thread_block(unsigned int thread_id, last_thread *lt);

inline void
end_thread_block(unsigned int thread_id, last_thread *lt);

#endif
