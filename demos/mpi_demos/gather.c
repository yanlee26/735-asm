#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

#define NDATA 5

int main(int argc, char *argv[])
{
  int sendarray[NDATA];
  int *recvbuf;
  int numproc, myid, i, N, dest;

  // Data from other nodes will be "gathered" to this node
  dest = 0;

  MPI_Init(&argc,&argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numproc);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);

  // Fill up the array with data to send to the destination node. Note
  // that the contents of the array will depend on the process ID
  for (i = 0; i < NDATA; ++i) sendarray[i] = 100 * myid + i;

  recvbuf = (int*)malloc(NDATA * numproc * sizeof(int));

  MPI_Gather(sendarray, NDATA, MPI_INT, recvbuf, NDATA, MPI_INT, dest, MPI_COMM_WORLD);

  if (myid == 0) {
    for (i = 0; i < NDATA * numproc; ++i) {
      printf("%d %d\n", i, recvbuf[i]);
    }
  }

  MPI_Finalize();
  free(recvbuf);
}
