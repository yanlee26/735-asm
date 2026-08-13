//first.c Adding numbers using two nodes

#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

// Always use argc and argv, as mpirun will pass the appropriate parms.
int main(int argc, char* argv[])
{
  int N, i, result, sum0, sum1, myid;
  
  // Status variable so that operations can be checked
  MPI_Status stat;
  
  // Initialize - must do this before calling any other MPI function
  MPI_Init(&argc,&argv);
  
  // Which node is this?
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);

  // Do this block if master process
  if (myid == 0) {

    // Get the number from the user
    N = atoi(argv[1]);

    // Master sends 'N' to slave
    MPI_Send(&N, 1, MPI_INT, 1,0, MPI_COMM_WORLD);

    // Partial result for node 0
    sum0=0;
    for(i=1;i<=N/2;i++){
      sum0=sum0+i;
    }
    result=sum0;

    // Master waits to receive 'sum1' from slave
    MPI_Recv(&sum1, 1, MPI_INT, 1,0, MPI_COMM_WORLD, &stat);

    // Adds the two partial results
    result=sum0+sum1;
    fprintf(stdout,"The final result is %d \n",result);
  }

  // Do this block if slave process
  else if (myid == 1) {

    // Slave waits to receive 'N' from master
    MPI_Recv(&N, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &stat);

    // Partial result for the slave
    sum1=0;
    for(i=N/2+1;i<=N;i++){
      sum1=sum1+i;
    }

    // Slave sends 'sum1' to master
    MPI_Send(&sum1, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
  }

  MPI_Finalize();
}
