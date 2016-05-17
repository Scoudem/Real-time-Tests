#include "rt_util.h"

void
schedule(pid_t pid, int policy, const struct sched_param param)
{
    if(sched_setscheduler(pid, policy, &param) == -1)
    {
        perror("sched_setscheduler failed");
        exit(-1);
    }
}

void
increment_time(struct timespec *t, unsigned long tm)
{
    t->tv_sec += tm;
}

void
increment_time_u(struct timespec *t, unsigned long tm)
{
    t->tv_nsec += tm;
}


void
copy_time(struct timespec *src, struct timespec *dest)
{
        dest->tv_nsec = src->tv_nsec;
        dest->tv_sec = src->tv_sec;
}

void
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
create_thread_params(unsigned int priority, unsigned int delay,
                     unsigned long ndelay, unsigned long interval,
                     unsigned char thread_id, last_thread *lt,
                     thread_pool *pool)
{
    thread_params *tp;

    tp = (struct thread_params *) malloc(sizeof(struct thread_params));
    tp->priority = priority;
    tp->delay = delay;
    tp->ndelay = ndelay;
    tp->interval = interval;
    tp->thread_id = thread_id;
    tp->lt = lt;
    tp->tp = pool;

    return tp;
}

last_thread *
create_last_thread()
{
    last_thread *lt;

    lt = (struct last_thread *) malloc(sizeof(struct last_thread));
    lt->start_time = INVALID_TIME;
    lt->end_time = INVALID_TIME;
    lt->thread_id = INVALID_THREAD;
    lt->state = JOB_STATE_DONE;

    /* Creates a mutex with the PTHREAD_PRIO_INHERIT attribute.
       This attribute is needed so that switches can happen based on
       priority of the thread */
    pthread_mutexattr_t attributes;
    pthread_mutexattr_init(&attributes);
    pthread_mutexattr_setprotocol(&attributes, PTHREAD_PRIO_INHERIT);
    //pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_init(&lt->mutex, &attributes);

    return lt;
}

struct thread_pool *
create_thread_pool()
{
    thread_pool *tp;

    tp = (struct thread_pool *) malloc(sizeof(struct thread_pool));

    tp->input  = (unsigned char *) calloc(DATA_SIZE, sizeof(unsigned char));
    tp->output = (unsigned char *) calloc(DATA_SIZE, sizeof(unsigned char));
    tp->count  = (unsigned long *) calloc(BIN_SIZE,  sizeof(unsigned long));

    return tp;
}

/*
 * Dump thread data to stderr. Can be wrong because
 * thread data is shared.
 */
void
dump_last_thread_data(unsigned int thread_id, char *message, last_thread *lt)
{
    fprintf(
        stderr,
        "Thread %d: %s.\n"
        "Dumping last known data (can be wrong):\n"
        " - start time:  %lu\n"
        " - end time:    %lu\n"
        " - last thread: %d\n"
        " - last state:  %d\n"
        "Dump finished, stopping thread...\n",
        thread_id, message, lt->start_time, lt->end_time,
        lt->thread_id, lt->state
    );
}

void
begin_thread_block(unsigned int thread_id, last_thread *lt)
{
    if (pthread_mutex_trylock(&lt->mutex) != 0)
    {
        /* We were unable to achieve a lock, that probably means that
           the previous task was not done. */
        dump_last_thread_data(thread_id, "unable to achieve lock", lt);

        lt->state = JOB_STATE_ERROR;
        pthread_exit((void*)-1);
    };

    /* Check the last job state */
    JOB_STATE state = lt->state;

    if (state != JOB_STATE_DONE)
    {
        /* This thread detected that the previous thread was not done. */
        dump_last_thread_data(thread_id, "previous thread not done", lt);

       lt->state = JOB_STATE_ERROR;
       pthread_exit((void*)-1);
    }

    struct timespec ets;
    clock_gettime(CLOCK_MONOTONIC, &ets);

    /* All clear, set our own state */
    lt->state = JOB_STATE_BUSY;
    lt->start_time = ets.tv_nsec;
    lt->end_time = INVALID_TIME;
    lt->thread_id = thread_id;
    pthread_mutex_unlock(&lt->mutex);
}

void
end_thread_block(unsigned int thread_id, last_thread *lt)
{
    if (pthread_mutex_trylock(&lt->mutex) != 0)
    {
        /* We were unable to achieve a lock, that probably means that
           the previous task was not done. */
        dump_last_thread_data(thread_id, "unable to achieve lock", lt);

        lt->state = JOB_STATE_ERROR;
        pthread_exit((void*)-1);
    };

    struct timespec ets;
    clock_gettime(CLOCK_MONOTONIC, &ets);
    unsigned long end_time = ets.tv_nsec;

    /* All clear, set our own state */
    lt->state = JOB_STATE_DONE;
    lt->end_time = end_time;
    pthread_mutex_unlock(&lt->mutex);

    printf("Job %d took %lu nsec\n", thread_id, lt->end_time - lt->start_time);
}
