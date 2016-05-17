#include "execute.h"

#include <pthread.h>
#include "rt_util.h"

void *
execute(void *arg)
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

    //stack_prefault();

    clock_gettime(CLOCK_MONOTONIC, &last_time);

    last_time.tv_sec += DELAY;

    printf("%p\n", pool);

    while(1)
    {
        /* Wait untill next shot */
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &last_time, NULL);

        begin_thread_block(THREAD_ID, lt);

        /* Processing */

        /* Reset to 0 */
        memset(pool->count, 0, BIN_SIZE);

        /* Fill bins */
        for (int index = 0; index < DATA_SIZE; index++)
        {
            pool->count[pool->output[index]] += 1;
        }

        unsigned char largest = 0;
        for(int index = 0; index < BIN_SIZE; index++)
        {
            if (pool->count[index] > pool->count[largest])
                largest = index;
        }

        fprintf(stdout, "largest bin is %d with %d entries\n",
                largest, pool->count[largest]);

        end_thread_block(THREAD_ID, lt);

        /* Calculate next shot */
        increment_time_u(&last_time, INTERVAL);
        normalise_time(&last_time);

    }

    return NULL;
}
