#include "generate.h"

#include <pthread.h>
#include <stdlib.h>
#include "rt_util.h"

static int compare_char(const void *this, const void *that)
{
    char *ti = (char*) this;
    char *ta = (char*) that;
    return *ti - *ta;
}

void *
sort(void *arg)
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
    ts->times_sorted = 0;

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
    printf("Started thread %d @ %zu\n", THREAD_ID, last_time.tv_nsec);

    stack_prefault();

    if (DELAY > -1)
        last_time.tv_sec += DELAY;
    else
        last_time.tv_nsec = NDELAY;

    normalise_time(&last_time);

    while(1)
    {
        /* Wait untill next shot */
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &last_time, NULL);

        unsigned int result = begin_thread_block(THREAD_ID, lt, 0);
        if (!result)
            break;

        /* Processing */
        unsigned char *target = NULL;
        if (pool->current == 0)
            target = pool->input;
        else
            target = pool->output;

        if (pool->dirty)
        {
            printf("Sorting %s\n", pool->current ? "output" : "input");
            ts->times_sorted += 1;
            qsort(target, DATA_SIZE, sizeof(*target), compare_char);
            pool->dirty = 0;
        }

        /* Sorting does not need all checks since it is in between */
        // ts->average_time += end_thread_block(THREAD_ID, INTERVAL, lt, 0);

        /* Calculate next shot */
        increment_time_u(&last_time, INTERVAL);
        normalise_time(&last_time);
    }

    ts->average_time = ts->average_time / lt->max_iterations;
    pthread_exit(ts);

    return NULL;
}
