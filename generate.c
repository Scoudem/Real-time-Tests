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

    last_time.tv_sec += DELAY;

    while(1)
    {
        /* Wait untill next shot */
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &last_time, NULL);

        begin_thread_block(THREAD_ID, lt);

        /* Processing */
        fprintf(stdout, "Hello from GENERATE @ %lld\n", (long long) last_time.tv_sec);

        end_thread_block(THREAD_ID, lt);

        /* Calculate next shot */
        increment_time_u(&last_time, INTERVAL);
        normalise_time(&last_time);

    }

    return NULL;
}
