#include <stdio.h>
#include <math.h>
#include <mpi.h>

#define PI 3.1415926535

int main(int argc, char **argv)
{
  long long i, num_intervals;
  double rect_width, area, sum, x_middle;
  double t_sum;

  int comm_size, comm_rank;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);

  sscanf(argv[1],"%llu",&num_intervals);

  rect_width = PI / num_intervals;

  t_sum = 0;
  for(i = 1+comm_rank; i < num_intervals + 1; i+=comm_size) {

    /* find the middle of the interval on the X-axis. */

    x_middle = (i - 0.5) * rect_width;
    area = sin(x_middle) * rect_width;
    t_sum = t_sum + area;
  }

  // get sum using sum op.
  MPI_Reduce(&t_sum, &sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  if (comm_rank == 0)
      printf("The total area is: %f\n", (float)sum);

  MPI_Finalize();

  return 0;
}
