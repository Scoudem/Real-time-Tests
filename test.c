#include <stdio.h>
#include <pthread.h>
#include <sched.h>

#include "rt_util.h"
#include "generate.h"
#include "process.h"
#include "execute.h"
#include "sort.h"


#define thread_start(THREAD, P_ARG, FUNC, ARG)\
if (pthread_create(THREAD, P_ARG, FUNC, ARG)) {\
    fprintf(stderr, "Error creating thread\n");\
    return 1;\
}

#define thread_join(THREAD, RET)\
if (pthread_join(THREAD, RET)) {\
    fprintf(stderr, "Error joining thread\n");\
    return 2;\
}

#define INT_TIME 5000000

int
main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Not enough arguments. Usage: ./test <interval in nanosec> <num_tests>\n");
        exit(1);
    }

    int interval  = atoi(argv[1]),
        num_tests = atoi(argv[2]);

    fprintf(stdout, "Starting execution.\n");

    pthread_t thread_generate,
              thread_process,
              thread_execute,
              thread_sort;

    thread_params *params_generate,
                  *params_process,
                  *params_execute,
                  *params_sort;

    last_thread *lt   = create_last_thread(num_tests);
    thread_pool *pool = create_thread_pool();

    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);

    int max_prio = sched_get_priority_max(SCHED_FIFO);
    params_generate = create_thread_params(max_prio, -1,
                                           current_time.tv_nsec + interval * 10, interval * 3,
                                           1, lt, pool);
    params_process  = create_thread_params(max_prio, -1,
                                           current_time.tv_nsec + interval * 11, interval * 3,
                                           2, lt, pool);
    params_execute  = create_thread_params(max_prio, -1,
                                           current_time.tv_nsec + interval * 12, interval * 3,
                                           3, lt, pool);
    params_sort     = create_thread_params(max_prio - 1, -1,
                                           current_time.tv_nsec + interval * 10, 10000,
                                           4, lt, pool);

    thread_start(&thread_generate, NULL, &generate, params_generate);
    thread_start(&thread_process,  NULL, &process, params_process);
    thread_start(&thread_execute,  NULL, &execute, params_execute);
    thread_start(&thread_sort,     NULL, &sort, params_sort);

    thread_stats *ts1, *ts2, *ts3, *ts4;

    thread_join(thread_generate, (void**)&ts1);
    fprintf(stdout, "Thread 1 joined.\n");
    thread_join(thread_process, (void**)&ts2);
    fprintf(stdout, "Thread 2 joined.\n");
    thread_join(thread_execute, (void**)&ts3);
    fprintf(stdout, "Thread 3 joined.\n");
    thread_join(thread_sort, (void**)&ts4);
    fprintf(stdout, "Thread 4 joined.\n");

    free(params_generate);
    free(params_process);
    free(params_execute);
    free(params_sort);

    fprintf(stdout, "Execution finished.\n");

    fprintf(stdout, "Thread generate average run time: %llu nsec\n", ts1->average_time);
    fprintf(stdout, "Thread process average run time: %llu nsec\n", ts2->average_time);
    fprintf(stdout, "Thread execute average run time: %llu nsec\n", ts3->average_time);
    fprintf(stdout, "Thread sort times sorted: %d\n", ts4->times_sorted);

    return 0;
}
