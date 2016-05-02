#include "rt_util.h"

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

inline void
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

thread_params *
create_thread_params(int priority, int delay, long ndelay, long interval)
{
    thread_params *tp;

    tp = (struct thread_params *) malloc(sizeof(struct thread_params));
    tp->priority = priority;
    tp->delay = delay;
    tp->ndelay = ndelay;
    tp->interval = interval;

    return tp;
}
