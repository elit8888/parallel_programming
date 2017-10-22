/*
 * Simple Pi program
 * Calculate pi value using Monte-Carlo method.
 * It accepts two positional arguments: # of cpu, # of iterations.
 * It assumes that one thread is used for each cpu.
 */
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h> // strtol
#include <time.h>   // time seed

typedef long long int llint;

/* Function for threads */
void *thread_run(void *param);

/* Global variables */
llint toss_hit_count = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


int main(int argc, char * argv[])
{
    // Get # of cpus and # of iterations.
    if (argc != 3)
    {
        printf("Usage: %s <# of cpu> <# of iter>.\n", argv[0]);
        exit(1);
    }
    int rc; // return code for pthread_create
    long n_cpu;
    llint n_iter, *thread_iter = (llint *)malloc(sizeof(llint));
    n_cpu = strtol(argv[1], NULL, 10);
    n_iter = strtoll(argv[2], NULL, 10);
    *thread_iter = n_iter / n_cpu;
    pthread_t *threads = (pthread_t *)malloc(n_cpu * sizeof(pthread_t));
    srand((unsigned)time(NULL));    // use time for seed

#if DEBUG
    printf("%ld cpus, %lld iterations.\n", n_cpu, n_iter);

    // time_t start = time(NULL);   // the precision is only to sec
    struct timespec requestStart, requestEnd;
    clock_gettime(CLOCK_REALTIME, &requestStart);
#endif

    // start threading
    for (int i = 0; i < n_cpu; ++i)
    {
        rc = pthread_create(&threads[i], NULL, thread_run, (void *)thread_iter);
        if (rc)
        {
            printf("ERROR thread create with return code %d.\n", rc);
            exit(EXIT_FAILURE);
        }
    }

    // join threads
    for (int i = 0; i < n_cpu; ++i)
        pthread_join(threads[i], NULL);

    double pi_estimate = toss_hit_count * 4.0 / n_iter;

#if DEBUG
    clock_gettime(CLOCK_REALTIME, &requestEnd);
    double accum = (requestEnd.tv_sec - requestStart.tv_sec)
        + (requestEnd.tv_nsec - requestStart.tv_nsec) / 1E9;
    printf("%f, time: %lf.\n", pi_estimate, accum);
#else
    printf("%f\n", pi_estimate);
#endif

    free(threads);
    free(thread_iter);
    return 0;
}


void *thread_run(void *param)
{
    double x, y;
    llint n_hit = 0, t_iter = *(llint *)param;
    unsigned int rand_state = rand(); // use rand_r because rand won't speed up

    for (llint i = 0; i != t_iter; ++i)
    {
        x = (double)(rand_r(&rand_state)) / (RAND_MAX) * 2 - 1;
        y = (double)(rand_r(&rand_state)) / (RAND_MAX) * 2 - 1;
        if ((x*x + y*y) <= 1)   ++n_hit;
    }

#if DEBUG
    printf("%lld / %lld\n", n_hit, t_iter);
#endif

    pthread_mutex_lock(&mutex);
    toss_hit_count += n_hit;
    pthread_mutex_unlock(&mutex);

    pthread_exit(NULL);
}
