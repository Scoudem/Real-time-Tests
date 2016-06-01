#include <stdio.h>
#include <pthread.h>
#include <sched.h>

#include "rt_util.h"
#include "generate.h"
#include "process.h"
#include "execute.h"

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
        fprintf(stderr, "Not enough arguments. Usage: ./test <interval> <num_tests>\n");
        exit(1);
    }

    int interval  = atoi(argv[1]),
        num_tests = atoi(argv[2]);

    fprintf(stdout, "Starting execution.\n");

    pthread_t thread_generate,
              thread_process,
              thread_execute;

    thread_params *params_generate,
                  *params_process,
                  *params_execute;

    last_thread *lt   = create_last_thread(num_tests);
    thread_pool *pool = create_thread_pool();

    int max_prio = sched_get_priority_max(SCHED_FIFO);
    params_generate = create_thread_params(max_prio, -1,
                                           interval, interval * 3,
                                           1, lt, pool);
    params_process  = create_thread_params(max_prio, -1,
                                           interval * 2, interval * 3,
                                           2, lt, pool);
    params_execute  = create_thread_params(max_prio, -1,
                                           interval * 3, interval * 3,
                                           3, lt, pool);

    thread_start(&thread_generate, NULL, &generate, params_generate);
    thread_start(&thread_process, NULL, &process, params_process);
    thread_start(&thread_execute, NULL, &execute, params_execute);

    thread_join(thread_generate, NULL);
    fprintf(stdout, "Thread 1 joined.\n");
    thread_join(thread_process, NULL);
    fprintf(stdout, "Thread 2 joined.\n");
    thread_join(thread_execute, NULL);
    fprintf(stdout, "Thread 3 joined.\n");

    free(params_generate);
    free(params_process);
    free(params_execute);

    fprintf(stdout, "Execution finished.\n");

    return 0;
}
