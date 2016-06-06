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

#define MAX_SAFE_STACK  (32*1024)
#define NSEC_PER_SEC    (1000000000)

#define DATA_SIZE       (12*1024)
#define BIN_SIZE        256

#define INVALID_TIME    0
#define INVALID_THREAD  0x0

typedef enum JOB_STATE
{
    JOB_STATE_UNKOWN,
    JOB_STATE_DONE,
    JOB_STATE_BUSY,
    JOB_STATE_ERROR,
} JOB_STATE;

typedef struct last_thread
{
    pthread_mutex_t mutex;
    struct timespec *start_time;
    struct timespec *end_time;
    unsigned char thread_id;
    JOB_STATE state;
    unsigned int max_iterations;
    unsigned int current_iterations;
} last_thread;

typedef struct thread_pool
{
    unsigned char *input;
    unsigned char *output;
    unsigned long *count;
    unsigned char current;
    unsigned char dirty;
} thread_pool;

typedef struct thread_params
{
    unsigned int priority;
    unsigned int delay;
    unsigned long ndelay;
    unsigned long interval;
    unsigned char thread_id;
    last_thread *lt;
    thread_pool *tp;
} thread_params;

typedef struct thread_stats
{
    unsigned long long average_time;
    unsigned int times_sorted;
} thread_stats;

void
schedule(pid_t pid, int policy, const struct sched_param param);

void
increment_time(struct timespec *t, unsigned long tm);

void
increment_time_u(struct timespec *t, unsigned long tm);

void
copy_time(struct timespec *src, struct timespec *dest);

void
normalise_time(struct timespec *t);

void
stack_prefault(void);

struct thread_params *
create_thread_params(unsigned int priority, unsigned int delay,
                     unsigned long ndelay, unsigned long interval,
                     unsigned char thread_id, last_thread *lt,
                     thread_pool *pool);

struct last_thread *
create_last_thread(unsigned int iterations);

struct thread_pool *
create_thread_pool();

void
dump_last_thread_data(unsigned int thread_id, char *message, last_thread *lt);

unsigned int
begin_thread_block(unsigned int thread_id, last_thread *lt, int do_checks);

unsigned long
end_thread_block(unsigned int thread_id, unsigned long interval, last_thread *lt, int do_checks);

#endif
