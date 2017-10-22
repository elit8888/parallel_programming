#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h> // strtol
#include <time.h>   // time seed

#define Rand(low,up) (double)(rand()) / (RAND_MAX) * (up - low) + low

/* function for threads */
void *thread_foo(void *param);

int n_iter, n_cpu;
int *result;

int main(int argc, char * argv[])
{
    // Get # of cpus and # of iterations.
    if (argc != 3)
    {
        printf("Usage: %s <# of cpu> <# of iter>.\n", argv[0]);
        exit(1);
    }
    n_cpu = strtol(argv[1], NULL, 10);
    n_iter = strtol(argv[2], NULL, 10);
#if DEBUG
    printf("%d cpus, %d iterations.\n", n_cpu, n_iter);
#endif

    // set timer
    // time_t start = time(NULL);
    struct timespec requestStart, requestEnd;
    clock_gettime(CLOCK_REALTIME, &requestStart);

    // Start estimate pi
    srand((unsigned)time(NULL));    // use time for seed

    // start threading
    int t = 0;
    long thread_iter = n_iter / n_cpu;
    double pi_estimate = 0;
    pthread_attr_t attr;
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_t *threads = (pthread_t *)malloc(n_cpu * sizeof(pthread_t));
    result = (int *)malloc(n_cpu * sizeof(int));

    for (int i = 0; i < n_cpu; ++i)
    {
        t = pthread_create(&threads[i], NULL, thread_foo, (void *)i);
        if (t)
        {
            printf("ERROR thread create with return code %d.\n", t);
        }
    }

    // collect result
    void *status;
    int total_cnt = 0;
    for (int i = 0; i < n_cpu; ++i)
    {
        pthread_join(threads[i], NULL);
        total_cnt += result[i];
    }

    // print result
    pi_estimate = 4.0 * total_cnt / n_iter;
    clock_gettime(CLOCK_REALTIME, &requestEnd);
    double accum = (requestEnd.tv_sec - requestStart.tv_sec)
        + (requestEnd.tv_nsec - requestStart.tv_nsec) / 1E9;
    printf("%f, time: %lf.\n", pi_estimate, accum);
    return 0;
}


void *thread_foo(void *param)
{
    double x, y;
    int n_hit = 0, n_max = RAND_MAX;
    int t_iter = n_iter / n_cpu;
    unsigned int rand_state = rand(); // use rand_r because randa won't speed up

    for (int i = 0; i != t_iter; ++i)
    {
        x = (double)(rand_r(&rand_state)) / (n_max) * 2 - 1;
        y = (double)(rand_r(&rand_state)) / (n_max) * 2 - 1;
        if ((x*x + y*y) <= 1)   ++n_hit;
    }
    result[(int)param] = n_hit;

    pthread_exit(NULL);
}
