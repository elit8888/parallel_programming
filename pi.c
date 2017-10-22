#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h> // strtol
#include <time.h>   // time seed


/* function for threads */
void *thread_foo(void *param);

int toss_hit_count = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char * argv[])
{
    // Get # of cpus and # of iterations.
    if (argc != 3)
    {
        printf("Usage: %s <# of cpu> <# of iter>.\n", argv[0]);
        exit(1);
    }
    int n_iter, n_cpu, rc;
    n_cpu = strtol(argv[1], NULL, 10);
    n_iter = strtol(argv[2], NULL, 10);
    long thread_iter = n_iter / n_cpu;
    pthread_t *threads = (pthread_t *)malloc(n_cpu * sizeof(pthread_t));

#if DEBUG
    printf("%d cpus, %d iterations.\n", n_cpu, n_iter);

    // time_t start = time(NULL);   // the precision is only to sec
    struct timespec requestStart, requestEnd;
    clock_gettime(CLOCK_REALTIME, &requestStart);
#endif

    srand((unsigned)time(NULL));    // use time for seed

    // start threading
    for (int i = 0; i < n_cpu; ++i)
    {
        rc = pthread_create(&threads[i], NULL, thread_foo, (void *)thread_iter);
        if (rc)
        {
            printf("ERROR thread create with return code %d.\n", rc);
            exit(EXIT_FAILURE);
        }
    }

    // join threads
    for (int i = 0; i < n_cpu; ++i)
        pthread_join(threads[i], NULL);

    // print result
    double pi_estimate = 4.0 * toss_hit_count / n_iter;

#if DEBUG
    clock_gettime(CLOCK_REALTIME, &requestEnd);
    double accum = (requestEnd.tv_sec - requestStart.tv_sec)
        + (requestEnd.tv_nsec - requestStart.tv_nsec) / 1E9;
    printf("%f, time: %lf.\n", pi_estimate, accum);
#else
    printf("%f\n", pi_estimate);
#endif

    return 0;
}


void *thread_foo(void *param)
{
    double x, y;
    int n_hit = 0, n_max = RAND_MAX;
    long t_iter = (long)param;
    unsigned int rand_state = rand(); // use rand_r because randa won't speed up

    for (int i = 0; i != t_iter; ++i)
    {
        x = (double)(rand_r(&rand_state)) / (n_max) * 2 - 1;
        y = (double)(rand_r(&rand_state)) / (n_max) * 2 - 1;
        if ((x*x + y*y) <= 1)   ++n_hit;
    }
    pthread_mutex_lock(&mutex);
    toss_hit_count += n_hit;
    pthread_mutex_unlock(&mutex);

    pthread_exit(NULL);
}
