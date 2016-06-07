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
#define NUM_PARTS       1000

#define INVALID_TIME    0
#define INVALID_THREAD  0x0

#define INTERVAL_GENERATE   2000000
#define INTERVAL_PROCESS    1000000
#define INTERVAL_EXECUTE     500000
#define INTERVAL_SORT          5000
#define INTERVAL_IS_SORT       5000
#define TOTAL_INTERVAL INTERVAL_GENERATE + INTERVAL_PROCESS + INTERVAL_EXECUTE

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
    unsigned char is_sorted;
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
    unsigned long long interval;
    unsigned long long min_time;
    unsigned long long max_time;
    unsigned long long average_time;
    unsigned long long min_jitter;
    unsigned long long max_jitter;
    unsigned long long average_jitter;
    unsigned long min_wrong;
    unsigned long max_wrong;
    unsigned long average_wrong;
    unsigned int times_sorted;
    unsigned int times_is_sorted;
} thread_stats;

void
schedule(pid_t pid, int policy, const struct sched_param param);

void
increment_time(struct timespec *t, unsigned long tm);

void
increment_time_u(struct timespec *t, unsigned long tm);

void
copy_time(struct timespec *src, struct timespec *dest);

long long
between_time(struct timespec *early, struct timespec *late);

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
