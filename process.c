#include "process.h"

#include <pthread.h>
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

    struct timespec last_time;
    struct sched_param param;

    param.sched_priority = PRIORITY;
    schedule(0, SCHED_FIFO, param);

    /* Lock memory */
    if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
        perror("mlockall failed");
        exit(-2);
    }

    printf("Started thread %d\n", THREAD_ID);

    clock_gettime(CLOCK_MONOTONIC, &last_time);

    if (DELAY > -1)
        last_time.tv_sec += DELAY;
    else
        last_time.tv_nsec += NDELAY;

    while(1)
    {
        /* Wait untill next shot */
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &last_time, NULL);

        unsigned int result = begin_thread_block(THREAD_ID, lt);
        if (!result)
            break;

        /* Processing */

        for (int index = 0; index < DATA_SIZE; index++)
        {
            pool->output[index] = pool->input[index];
        }

        ts->average_time += end_thread_block(THREAD_ID, INTERVAL, lt);

        /* Calculate next shot */
        increment_time_u(&last_time, INTERVAL);
        normalise_time(&last_time);
    }

    ts->average_time = ts->average_time / lt->max_iterations;
    pthread_exit(ts);

    return NULL;
}
