#include "generate.h"

#include <pthread.h>
#include "rt_util.h"

void *
generate(void *arg)
{
    thread_params *tp = (thread_params*)arg;

    unsigned char THREAD_ID = tp->thread_id;

    unsigned int PRIORITY   = tp->priority,
                 DELAY      = tp->delay;

    unsigned long INTERVAL  = tp->interval,
                  NDELAY    = tp->ndelay;

    last_thread *lt         = tp->lt;

    thread_pool *pool       = tp->tp;

    struct timespec last_time;
    struct sched_param param;

    param.sched_priority = PRIORITY;
    schedule(0, SCHED_FIFO, param);

    /* Lock memory */
    if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
        perror("mlockall failed");
        exit(-2);
    }

    srand(time(NULL));

    clock_gettime(CLOCK_MONOTONIC, &last_time);

    printf("Started thread %d\n", THREAD_ID);

    if (DELAY > -1)
        last_time.tv_sec += DELAY;
    else
        last_time.tv_nsec += NDELAY;

    while(1)
    {
        /* Wait untill next shot */
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &last_time, NULL);

        begin_thread_block(THREAD_ID, lt);

        /* Processing */
        for (int index = 0; index < DATA_SIZE; index++)
        {
            pool->input[index] = rand() % BIN_SIZE;
        }

        end_thread_block(THREAD_ID, INTERVAL, lt);

        /* Calculate next shot */
        increment_time_u(&last_time, INTERVAL);
        normalise_time(&last_time);

    }

    return NULL;
}
