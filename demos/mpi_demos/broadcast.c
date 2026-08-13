/*
  Broadcast example: 

  This also implements the partial sum problem from second.c/cpp and
  introduces the Reduce operation.

*/
#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

int main(int argc, char *argv[])
{
  int numproc, myid, namelen, result, sum=0, i, N=0;
  char processor_name[MPI_MAX_PROCESSOR_NAME];

  MPI_Init(&argc,&argv); //INITIALIZE
  MPI_Comm_size(MPI_COMM_WORLD, &numproc); //how many processors??
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);    //what is THIS processor-ID?

  //what is THIS processor name (hostname)?
  MPI_Get_processor_name(processor_name,&namelen);

  if(argc<2) {printf("No argument\n");MPI_Finalize();exit(0);}

  // If master process, get N from user input
  if (myid == 0) N = atoi(argv[1]);
  printf("id=%d N=%d before broadcast\n", myid, N);

  // Broadcast from master to all slaves. If this is the master
  // prcess, then this routine is sending, if this is a slave process,
  // then the routine is receiving
  MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);
  printf("id=%d N=%d after broadcast\n", myid, N);

  // Note here we don't need separate branching for master/slave operations
  int first = N/numproc * myid + 1;
  int last  = N/numproc * (myid+1);
  for (i=first; i <= last; ++i) sum +=i;

  // Combine the sums computed on each node, into result. If this is
  // one of the slaves, this routine will send sum to the master. If
  // this is the master process, then it will receive the sums and
  // combine them.
  MPI_Reduce(&sum, &result, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
  printf("id=%d reduce %d %d\n", myid, sum, result);

  if (myid==0) printf("Sum= %d\n", result);

  MPI_Finalize();
}
