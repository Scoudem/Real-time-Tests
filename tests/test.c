#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sched.h>
#include <sys/mman.h>
#include <string.h>

#define MY_PRIORITY     (49)
#define MAX_SAFE_STACK  (8*1024)
#define NSEC_PER_SEC    (1000000000)

inline void
schedule(pid_t pid, int policy, const struct sched_param param)
{
    if(sched_setscheduler(pid, policy, &param) == -1)
    {
        perror("sched_setscheduler failed");
        exit(-1);
    }
}

inline void
increment_time(struct timespec *t, long tm)
{
    t->tv_nsec += tm;
}

inline void
copy_time(struct timespec *src, struct timespec *dest)
{
        dest->tv_nsec = src->tv_nsec;
        dest->tv_sec = src->tv_sec;
}

static inline void
normalise_time(struct timespec *t)
{
    while (t->tv_nsec >= NSEC_PER_SEC)
    {
        t->tv_nsec -= NSEC_PER_SEC;
        t->tv_sec += 1;
    }
}

void
stack_prefault(void)
{
    unsigned char dummy[MAX_SAFE_STACK];
    memset(dummy, 0, MAX_SAFE_STACK);
    return;
}

int
main(int argc, char* argv[])
{
    struct timespec last_time;
    struct sched_param param;
    int interval = 1000000; /* 1 sec */

    param.sched_priority = MY_PRIORITY;
    schedule(0, SCHED_FIFO, param);

    /* Lock memory */
    if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
        perror("mlockall failed");
        exit(-2);
    }

    /* Pre-fault our stack */
    stack_prefault();

    /* Get current clocktime */
    clock_gettime(CLOCK_MONOTONIC, &last_time);

    /* Start after 1 sec */
    last_time.tv_sec += 1;

    while(1)
    {
        /* Wait untill next shot */
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &last_time, NULL);

        /* Processing */
        printf("-> %lld\n", last_time.tv_sec);

        /* Calculate next shot */
        increment_time(&last_time, interval);
        normalise_time(&last_time);

    }
}
