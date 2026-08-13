#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

int main(int argc, char *argv[]) 
{
  int *recvdata;
  int *senddata;
  int numproc, myid, i, ndata;

  MPI_Init(&argc,&argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numproc);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);
  printf("I'm process %d out of %d\n", myid, numproc);

  senddata = (int*)malloc(numproc * sizeof(int));
  recvdata = (int*)malloc(numproc * sizeof(int));
  for (i = 0; i < numproc; ++i) senddata[i] = 1 + myid * numproc + i;

  printf("Before ID=%d : ", myid);
  for (i = 0; i < numproc; ++i) printf(" %d", senddata[i]);
  printf("\n");

  ndata = 1;
  MPI_Alltoall(senddata, ndata, MPI_INT, recvdata, ndata, MPI_INT, 
	       MPI_COMM_WORLD);

  printf("After  ID=%d : ", myid);
  for (i = 0; i < numproc; ++i) printf(" %d", recvdata[i]);
  printf("\n");

  MPI_Finalize();
  free(senddata);
  free(recvdata);
}
