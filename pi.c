#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h> // strtol
#include <time.h>   // time seed

#define Rand(low,up) (double)(rand()) / (RAND_MAX) * (up - low) + low

/* function for threads */
void *thread_foo(void *param);

int n_iter, n_cpu;

int main(int argc, char * argv[])
{
    // Get # of cpus and # of iterations.
    if (argc != 3)
    {
        printf("Usage: %s <n_cpu> <n_iter>.\n", argv[0]);
        exit(1);
    }
    n_cpu = strtol(argv[1], NULL, 10);
    n_iter = strtol(argv[2], NULL, 10);
#if DEBUG
    printf("%d cpus, %d iterations.\n", n_cpu, n_iter);
#endif

    // Start estimate pi
    srand((unsigned)time(NULL));    // use time for seed

    // start threading
    int t = 0, thread_iter = n_iter / n_cpu;
    double pi_estimate = 0;
    pthread_t *threads = (pthread_t *)malloc(n_cpu * sizeof(pthread_t));
    printf("Each thread need %d iters.\n", thread_iter);

    for (int i = 0; i < n_cpu; ++i)
    {
        t = pthread_create(&threads[i], NULL, thread_foo, (void *)thread_iter);
        if (t)
        {
            printf("ERROR thread create with return code %d.\n", t);
        }
    }

    void *status;
    int result = 0;
    for (int i = 0; i < n_cpu; ++i)
    {
        pthread_join(threads[i], &status);
        result += *(int *)status;
    }

    // print result
    pi_estimate = 4.0 * result / n_iter;
    printf("%f\n", pi_estimate);
    return 0;
}


void *thread_foo(void *param)
{
    int t_iter = (int)param;
    double x, y;
    int *n = (int *)malloc(sizeof(int));
    *n = 0;

    for (int i = 0; i != t_iter; ++i)
    {
        x = Rand(-1, 1);
        y = Rand(-1, 1);
        if ((x*x + y*y) <= 1)   *n += 1;
    }

    pthread_exit((void *)n);
}
