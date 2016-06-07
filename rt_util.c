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

long long
between_time(struct timespec *early, struct timespec *late)
{
    return (late->tv_sec  - early->tv_sec) * 1.0e9 +
           (late->tv_nsec - early->tv_nsec);
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
create_last_thread(unsigned int iterations)
{
    last_thread *lt;

    lt = (struct last_thread *) malloc(sizeof(struct last_thread));
    lt->start_time = malloc(sizeof(struct timespec));
    lt->end_time = malloc(sizeof(struct timespec));
    lt->thread_id = INVALID_THREAD;
    lt->state = JOB_STATE_DONE;
    lt->max_iterations = iterations;
    lt->current_iterations = 0;

    /* Creates a mutex with the PTHREAD_PRIO_INHERIT attribute.
       This attribute is needed so that switches can happen based on
       priority of the thread */
    pthread_mutexattr_t attributes;
    pthread_mutexattr_init(&attributes);
    pthread_mutexattr_setprotocol(&attributes, PTHREAD_PRIO_INHERIT);
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
    tp->current = 0;
    tp->dirty = 1;
    tp->is_sorted = 0;
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
        " - last thread: %d\n"
        " - last state:  %d\n"
        "Dump finished, stopping thread...\n",
        thread_id, message,
        lt->thread_id, lt->state
    );
}

unsigned int
begin_thread_block(unsigned int thread_id, last_thread *lt, int do_checks)
{

    if (do_checks)
    {
        if (pthread_mutex_trylock(&lt->mutex) != 0)
        {
            /* We were unable to achieve a lock, that probably means that
            the previous task was not done. */
            dump_last_thread_data(thread_id,
                "unable to achieve lock in begin_thread_block", lt);

            lt->state = JOB_STATE_ERROR;
            return 0;
        }
    }

    /* Check the last job state */
    JOB_STATE state = lt->state;

    if (do_checks)
    {
        switch (state)
        {
            case JOB_STATE_DONE:
            case JOB_STATE_UNKOWN:
                break;
            case JOB_STATE_BUSY:
                dump_last_thread_data(thread_id, "previous thread not done", lt);
                goto fail;
            case JOB_STATE_ERROR:
                dump_last_thread_data(thread_id, "previous thread failed", lt);
                goto fail;
                break;
            default:
            fail:
                if (do_checks)
                {
                    lt->state = JOB_STATE_ERROR;
                    pthread_mutex_unlock(&lt->mutex);
                }
                return 0;
            break;
        }
    }
    else if (state == JOB_STATE_ERROR)
    {
        return 0;
    }

    if (do_checks)
    {
        if (lt->thread_id != thread_id - 1 &&
            lt->thread_id != thread_id + 2 &&
            lt->thread_id != 0)
        {
            /* The previous thread is not the right one */
            dump_last_thread_data(thread_id, "A thread was skipped", lt);
            lt->state = JOB_STATE_ERROR;
            pthread_mutex_unlock(&lt->mutex);
            return 0;
        }
    }

    if (lt->current_iterations > lt->max_iterations)
    {
        if (do_checks)
        {
            lt->thread_id = thread_id;
            pthread_mutex_unlock(&lt->mutex);
        }

        return 0;
    }

    /* All clear, set our own state */
    if (do_checks)
    {
        lt->state = JOB_STATE_BUSY;
        clock_gettime(CLOCK_MONOTONIC, lt->start_time);
        lt->thread_id = thread_id;
        pthread_mutex_unlock(&lt->mutex);
    }

    return 1;
}

unsigned long
end_thread_block(unsigned int thread_id, unsigned long interval, last_thread *lt, int do_checks)
{
    if (pthread_mutex_trylock(&lt->mutex) != 0)
    {
        /* We were unable to achieve a lock, that probably means that
           the previous task was not done. */
        dump_last_thread_data(thread_id,
            "unable to achieve lock in end_thread_block", lt);

        lt->state = JOB_STATE_ERROR;
        return 0;
    }

    /* Get current time and substract start time to get elapsed time */
    clock_gettime(CLOCK_MONOTONIC, lt->end_time);
    unsigned long elapsed_time = (lt->end_time->tv_sec  - lt->start_time->tv_sec) * 1.0e9 +
                                 (lt->end_time->tv_nsec - lt->start_time->tv_nsec);

    if (elapsed_time > interval)
    {
        printf("FAIL: %lu > %lu\n", elapsed_time, interval);
        dump_last_thread_data(thread_id, "did not reach deadline", lt);
        lt->state = JOB_STATE_ERROR;
        return 0;
    }

    printf("%d: %d: %lu\n", thread_id, lt->current_iterations, elapsed_time);

    /* All clear, set our own state */
    lt->state = JOB_STATE_DONE;

    if (thread_id == 3)
        lt->current_iterations += 1;

    pthread_mutex_unlock(&lt->mutex);

    return elapsed_time;
}
