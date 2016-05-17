#include <stdio.h>
#include <pthread.h>

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

#define INT_TIME 400000

int
main(int argc, char *argv[])
{
    fprintf(stdout, "Starting execution.\n");

    pthread_t thread_generate,
              thread_process,
              thread_execute;

    thread_params *params_generate,
                  *params_process,
                  *params_execute;

    last_thread *lt   = create_last_thread();
    thread_pool *pool = create_thread_pool();

    params_generate = create_thread_params(40, -1, INT_TIME,     INT_TIME * 3, 1, lt, pool);
    params_process  = create_thread_params(40, -1, INT_TIME * 2, INT_TIME * 3, 2, lt, pool);
    params_execute  = create_thread_params(40, -1, INT_TIME * 3, INT_TIME * 3, 3, lt, pool);

    thread_start(&thread_generate, NULL, &generate, params_generate);
    thread_start(&thread_process, NULL, &process, params_process);
    thread_start(&thread_execute, NULL, &execute, params_execute);

    thread_join(thread_generate, NULL);
    thread_join(thread_process, NULL);
    thread_join(thread_execute, NULL);

    free(params_generate);
    free(params_process);
    free(params_execute);

    fprintf(stdout, "Execution finished.\n");

    return 0;
}
