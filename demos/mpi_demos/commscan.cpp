/*
  159.735 MPI demo: cscan and connect to all processes in COMM_WORLD

  Ian Bond
  Updated: 10/7/2025
 */
#include <iostream>
#include <mpi.h>

typedef unsigned long ULONG;

int main(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);
 
  int numproc, myid;
  MPI_Comm_size(MPI_COMM_WORLD, &numproc);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);
 
  ULONG imesg, irecv;

  MPI_Status status;
  
  if (myid == 0) {

    std::cout << "Total of " << numproc << " processors" << std::endl;
    imesg = 287346387366;
    std::cout << "P0 sending " << imesg << " to slaves" << std::endl;

    // Send message to each "slave" processes
    for  (int i = 1; i < numproc;i++) {
      MPI_Send(&imesg, 1, MPI_UNSIGNED_LONG, i, 0, MPI_COMM_WORLD);
    }	
  } 

  else {
    imesg = myid;
    MPI_Recv(&irecv, 1, MPI_UNSIGNED_LONG, 0, 0, MPI_COMM_WORLD, &status);
    std::cout << "P" << myid << " received " << irecv << " from P0" << std::endl;
  }

  MPI_Finalize();
}
