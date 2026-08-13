/*
  159.735 MPI demo: partial sum incorporating multiple slave nodes

  Ian Bond
  Updated: 10/7/2025
 */
#include <cstdlib>
#include <iostream>
#include "mpi.h"

int main(int argc, char *argv[])
{
  // Status variable, so operations can be checked
  MPI_Status status; 

  // Initialize
  MPI_Init(&argc,&argv);

  // Processor workpool information
  int numproc, myid;
  MPI_Comm_size(MPI_COMM_WORLD, &numproc); // how many processors??
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);    // what is THIS processor ID?

  // What is THIS processor name (hostname)?
  int namelen;
  char processor_name[MPI_MAX_PROCESSOR_NAME];
  MPI_Get_processor_name(processor_name,&namelen);
  std::cout << "C++ version. Processor ID " << myid << " " 
            << processor_name << " out of " <<  numproc << std::endl;;

  // Do this if master
  if (myid == 0) {

    long N = std::atoi(argv[1]);

    // Master sends N to all the slave processes
    for (int i = 1; i < numproc; ++i) {
      MPI_Send(&N, 1, MPI_LONG, i,0, MPI_COMM_WORLD);
    }	

    // Master does its own partial sum
    long sum0 = 0;
    for (int i = 1; i <= N/numproc; ++i){
      sum0=sum0+i;
    }

    long result = sum0;

    // Receive from all nodes and accumulate into sum
    for (int i = 1; i < numproc; ++i) {
      long sum1;
      MPI_Recv(&sum1, 1, MPI_LONG, i,0, MPI_COMM_WORLD, &status);
      result = result + sum1;

      // Report results from each node and show status info too
      std::cout << "Node: " << i << " " << result << " " << sum1
		<< " Status: "
		<< status.MPI_SOURCE << " "
		<< status.MPI_TAG << " "
		<< status.MPI_ERROR << std::endl;
    }
    std::cout << "The sum is " << result << std::endl;
  }

  // This is not the master
  else {
    long N;
    MPI_Recv(&N, 1, MPI_LONG, 0, 0, MPI_COMM_WORLD, &status);

    long sum1=0;
    for(int i = (N/numproc*myid)+1; i <= (N/numproc*(myid+1)); ++i){
      sum1=sum1+i;
    }	
    MPI_Send(&sum1, 1, MPI_LONG, 0, 0, MPI_COMM_WORLD);
  }

  MPI_Finalize();
}
