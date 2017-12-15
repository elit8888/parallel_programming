/**********************************************************************
 * DESCRIPTION:
 *   Serial Concurrent Wave Equation - C Version
 *   This program implements the concurrent wave equation
 *********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define MAXPOINTS 1000000
#define MAXSTEPS 1000000
#define MINPOINTS 20
#define PI 3.14159265
#define BLOCKSIZE 256

void check_param(void);
void init_line(void);
void update (void);
void printfinal (void);

int nsteps,                     /* number of time steps */
    tpoints,                /* total points along string */
    rcode;                      /* generic return code */
float  values[MAXPOINTS];     /* values at time t */

/**********************************************************************
 *  Checks input values from parameters
 *********************************************************************/
void check_param(void)
{
   char tchar[20];

   /* check number of points, number of iterations */
   while ((tpoints < MINPOINTS) || (tpoints > MAXPOINTS)) {
      printf("Enter number of points along vibrating string [%d-%d]: "
           ,MINPOINTS, MAXPOINTS);
      scanf("%s", tchar);
      tpoints = atoi(tchar);
      if ((tpoints < MINPOINTS) || (tpoints > MAXPOINTS))
         printf("Invalid. Please enter value between %d and %d\n", 
                 MINPOINTS, MAXPOINTS);
   }
   while ((nsteps < 1) || (nsteps > MAXSTEPS)) {
      printf("Enter number of time steps [1-%d]: ", MAXSTEPS);
      scanf("%s", tchar);
      nsteps = atoi(tchar);
      if ((nsteps < 1) || (nsteps > MAXSTEPS))
         printf("Invalid. Please enter value between 1 and %d\n", MAXSTEPS);
   }

   printf("Using points = %d, steps = %d\n", tpoints, nsteps);

}

/**********************************************************************
 *     Initialize points on line
 *********************************************************************/
void init_line(void)
{
   int j;
   float x, fac, k, tmp;

   /* Calculate initial values based on sine curve */
   fac = 2.0 * PI;
   k = 0.0;
   tmp = tpoints - 1;
   for (j = 0; j < tpoints; j++) {
      x = k/tmp;
      values[j] = sin (fac * x);
      k = k + 1.0;
   }
}

/**********************************************************************
 *      Calculate new values using wave equation
 *********************************************************************/
__global__
void do_math(float *values, int npoints, int niters)
{
   int myidx = blockIdx.x * BLOCKSIZE + threadIdx.x;
   if (myidx < npoints-1)
   {
       float dtime, c, dx, tau, sqtau;
       float ov, va, nv;

       dtime = 0.3;
       c = 1.0;
       dx = 1.0;
       tau = (c * dtime / dx);
       sqtau = tau * tau;
       va = values[myidx];
       ov = va;
       for (int i = 0; i < niters; i++)
       {
           nv = (2.0 * va) - ov + (sqtau * (-2.0)*va);  // Can't combine, the answer will be different.
           ov = va;
           va = nv;
       }
       values[myidx] = va;
   }
}

/**********************************************************************
 *     Update all values along line a specified number of times
 *********************************************************************/
void update()
{
   float *vd;
   int tt_size = tpoints * sizeof(float);
   cudaMalloc((void**)&vd, tt_size);
   cudaMemcpy(vd, values, tt_size, cudaMemcpyHostToDevice);

   // Determine GridSize and BlockSize
   int gridSize = (tpoints-1) / BLOCKSIZE + 1;
   //dim3 dimGrid(gridSize, 1);
   //dim3 dimBlock(BLOCKSIZE, 1);

   /* Update values for each time step */
   do_math<<<gridSize, BLOCKSIZE>>>(vd, tpoints, nsteps);
      /* Update points along line for this time step */
      /* Update old values with new values */
   cudaMemcpy(values, vd, tt_size, cudaMemcpyDeviceToHost);
   cudaFree(vd);
}

/**********************************************************************
 *     Print final results
 *********************************************************************/
void printfinal()
{
   int i;

   for (i = 0; i < tpoints; i++) {
      printf("%6.4f ", values[i]);
      if ((i+1)%10 == 0)
         printf("\n");
   }
}

/**********************************************************************
 *  Main program
 *********************************************************************/
int main(int argc, char *argv[])
{
    sscanf(argv[1],"%d",&tpoints);
    sscanf(argv[2],"%d",&nsteps);
    check_param();
    printf("Initializing points on the line...\n");
    init_line();
    printf("Updating all points for all time steps...\n");
    update();
    printf("Printing final results...\n");
    printfinal();
    printf("\nDone.\n\n");

    return 0;
}
