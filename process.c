#include "process.h"

#include <pthread.h>
#include <limits.h>
#include "rt_util.h"

void *
process(void *arg)
{
    thread_params *tp = (thread_params*)arg;

    unsigned char THREAD_ID = tp->thread_id;

    unsigned int PRIORITY   = tp->priority,
                 DELAY      = tp->delay;

    unsigned long INTERVAL  = tp->interval,
                  NDELAY    = tp->ndelay;

    last_thread *lt         = tp->lt;

    thread_pool *pool       = tp->tp;

    thread_stats *ts        = malloc(sizeof *ts);
    ts->interval = INTERVAL;
    ts->min_time = ULLONG_MAX;
    ts->max_time = 0;
    ts->min_jitter = ULLONG_MAX;
    ts->max_jitter = 0;

    struct timespec last_time;
    struct sched_param param;

    param.sched_priority = PRIORITY;
    schedule(0, SCHED_FIFO, param);

    /* Lock memory */
    if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
        perror("mlockall failed");
        exit(-2);
    }

    stack_prefault();

    clock_gettime(CLOCK_MONOTONIC, &last_time);
    printf("Started thread %d @ %zu\n", THREAD_ID, last_time.tv_nsec);

    last_time.tv_sec = DELAY;
    last_time.tv_nsec = NDELAY;
    normalise_time(&last_time);

    while(1)
    {
        /* Wait untill next shot */
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &last_time, NULL);

        /* Calculate next shot */
        increment_time_u(&last_time, INTERVAL);
        normalise_time(&last_time);

        struct timespec begin_time;
        clock_gettime(CLOCK_MONOTONIC, &begin_time);
        unsigned long elapsed_time = (begin_time.tv_sec  - lt->end_time->tv_sec) * 1.0e9 +
                                     (begin_time.tv_nsec - lt->end_time->tv_nsec);
        ts->average_jitter += elapsed_time;
        if (elapsed_time > ts->max_jitter) ts->max_jitter = elapsed_time;
        if (elapsed_time < ts->min_jitter) ts->min_jitter = elapsed_time;

        unsigned int result = begin_thread_block(THREAD_ID, lt, 1);
        if (!result)
            break;

        /* the current array is the processed one */
        pool->current = 1;

        /* Processing */
        for (int index = 0; index < DATA_SIZE; index++)
        {
            int result = 0;
            int item = pool->input[index];

            if (item < 64) result = 63;
            else if (item >= 64 && item < 128) result = 127;
            else if (item >= 128 && item < 192) result = 191;
            else if (item >= 192) result = 255;

            pool->output[index] = result;
            pool->dirty = 1;
        }

        elapsed_time = end_thread_block(THREAD_ID, INTERVAL, lt, 1);
        ts->average_time += elapsed_time;
        if (elapsed_time > ts->max_time) ts->max_time = elapsed_time;
        if (elapsed_time < ts->min_time) ts->min_time = elapsed_time;
    }

    ts->average_time = ts->average_time / lt->max_iterations;
    ts->average_jitter /= lt->max_iterations;
    pthread_exit(ts);

    return NULL;
}
