#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

#define NDATA 5

int main(int argc, char *argv[])
{
  int recvbuf[NDATA];
  int *sendbuf;
  int numproc, myid, i, N, root;
  
  // Data will be "scattered" from this node to the others
  root = 0;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numproc);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);

  // Fill up the array with data to send to the destination node.
  sendbuf = (int*)malloc(NDATA * numproc * sizeof(int));
  for (i = 0; i < NDATA * numproc; ++i) sendbuf[i] = i;

  MPI_Scatter(sendbuf, NDATA, MPI_INT, recvbuf, NDATA, MPI_INT, root, 
	      MPI_COMM_WORLD);

  for (i = 0; i < NDATA; ++i) {
    printf("%d %d %d\n", myid, i, recvbuf[i]);
  }

  MPI_Finalize();
  free(sendbuf);
}
