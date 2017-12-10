#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

int isprime(long long int n) {
  long long int i,squareroot;
  if (n>10) {
    squareroot = (long long int) sqrt(n);
    for (i=3; i<=squareroot; i=i+2)
      if ((n%i)==0)
        return 0;
    return 1;
  }
  else
    return 0;
}

int main(int argc, char *argv[])
{
  long long int pc,       /* prime counter */
                foundone; /* most recent prime found */
  long long int n, limit;

  int comm_size, comm_rank;
  long long int t_pc, t_foundone;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);
  sscanf(argv[1],"%llu",&limit);
  if (comm_rank == 0)
  {
    printf("Starting. Numbers to be scanned= %lld\n",limit);
  }


  //pc=4;     /* Assume (2,3,5,7) are counted here */
  t_pc = 0;
  t_foundone = 11;

#ifdef DEBUG
  printf("Check from %d, total num: %d, starting num: %d.\n",
          comm_rank, comm_size, 11+comm_rank);
#endif
  for (n=11+2*comm_rank; n<=limit; n=n+2*comm_size) {
#ifdef DEBUG
    if (comm_rank == 0) printf("Checking: %lld.\n", n);
#endif
    if (isprime(n)) {
      t_pc++;
      t_foundone = n;
    }
  }

  // get foundone using max op.
  MPI_Reduce(&t_foundone, &foundone, 1, MPI_LONG_LONG_INT, MPI_MAX,
                0, MPI_COMM_WORLD);
  // get pc using sum op.
  MPI_Reduce(&t_pc, &pc, 1, MPI_LONG_LONG_INT, MPI_SUM,
                0, MPI_COMM_WORLD);
  // add back the original result
  pc += 4;
  if (comm_rank == 0)
      printf("Done. Largest prime is %d Total primes %d\n",foundone,pc);

  MPI_Finalize();

  return 0;
}
