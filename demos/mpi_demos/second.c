#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

int main(int argc, char *argv[])
{
  int numproc, myid, namelen;

  long N;
  long i,result,sum0,sum1;
  char processor_name[MPI_MAX_PROCESSOR_NAME];

  MPI_Status Stat; //status variable, so operations can be checked

  MPI_Init(&argc,&argv);//INITIALIZE
  MPI_Comm_size(MPI_COMM_WORLD, &numproc); //how many processors??
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);    //what is THIS processor-ID?

  //what is THIS processor name (hostname)?
  MPI_Get_processor_name(processor_name,&namelen);
  fprintf(stdout, "Processor ID = %d: %s %d\n", myid, processor_name, numproc);

  // Do this if master
  if (myid == 0) {

    N = atoi(argv[1]);

    // Master sends N to all the slave processes
    for  (i=1;i<numproc;i++) {
      MPI_Send(&N, 1, MPI_LONG, i,0, MPI_COMM_WORLD);
    }	

    // Master does its own partial sum
    sum0 = 0;
    for(i=1;i<=N/numproc;i++){
      sum0=sum0+i;
    }
    result=sum0;
    for (i=1;i<numproc;i++) {//receive from all nodes
      MPI_Recv(&sum1, 1, MPI_LONG, i,0, MPI_COMM_WORLD, &Stat);
      result=result+sum1;//adds the various sums
      fprintf(stdout, "node=%ld: %ld %ld\n", i, result, sum1);
    }
    fprintf(stdout,"The sum from 1 to %ld is %ld \n",N,result);
  } 

  else {//this is not the master
    MPI_Recv(&N, 1, MPI_LONG, 0, 0, MPI_COMM_WORLD, &Stat);
    sum1=0;
    for(i=(N/numproc*myid)+1;i<=(N/numproc*(myid+1));i++){
      sum1=sum1+i;
    }	
    MPI_Send(&sum1, 1, MPI_LONG, 0, 0, MPI_COMM_WORLD);
  }


  MPI_Finalize();
}
