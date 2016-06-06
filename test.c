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

    /* Start after 1 second + interval * thread_id */
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    current_time.tv_sec += 1;

    int max_prio = sched_get_priority_max(SCHED_FIFO);
    params_generate = create_thread_params(max_prio, current_time.tv_sec,
                                           current_time.tv_nsec + interval * 1, interval * 3,
                                           1, lt, pool);
    params_process  = create_thread_params(max_prio, current_time.tv_sec,
                                           current_time.tv_nsec + interval * 2, interval * 3,
                                           2, lt, pool);
    params_execute  = create_thread_params(max_prio, current_time.tv_sec,
                                           current_time.tv_nsec + interval * 3, interval * 3,
                                           3, lt, pool);
    params_sort     = create_thread_params(max_prio - 1, current_time.tv_sec,
                                           current_time.tv_nsec + interval * 1, 1000,
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

    fprintf(stdout, "Completed %d iterations using %d items\n", num_tests, DATA_SIZE);
    fprintf(stdout, "All time values in nsec\n");
    fprintf(stdout, " - generate time (int, min, max, avg):\t%llu\t%llu\t%llu\t%llu\n",ts1->interval, ts1->min_time, ts1->max_time, ts1->average_time);
    fprintf(stdout, " - process time (int, min, max, avg): \t%llu\t%llu\t%llu\t%llu\n",ts2->interval, ts2->min_time, ts2->max_time, ts2->average_time);
    fprintf(stdout, " - Thread time (int, min, max, avg):  \t%llu\t%llu\t%llu\t%llu\n",ts3->interval, ts3->min_time, ts3->max_time, ts3->average_time);
    fprintf(stdout, "Times sorted: %d (%g every iteration)\n", ts4->times_sorted, (double)ts4->times_sorted/num_tests);

    return 0;
}
