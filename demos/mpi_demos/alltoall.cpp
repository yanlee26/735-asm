/*
  159.735 MPI demo: all_to_all collective communication pattern

  Ian Bond
  Updated: 10/7/2025
 */
#include <iostream>
#include "mpi.h"

int main(int argc, char *argv[]) 
{
  MPI_Init(&argc,&argv);
  
  int numproc, myid;
  MPI_Comm_size(MPI_COMM_WORLD, &numproc);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);

  // Number of data words sent per node-to-node communication. What
  // happens with other values > 1?
  const int ndata = 1;

  // Total amount of data
  const int ntotal = numproc * ndata;

  // Each nodes needs enough memory allocation for the sent and
  // received data
  int* senddata = new int[ntotal];
  int* recvdata = new int[ntotal];

  // Fill up the sending data with some values
  for (int i = 0; i < ntotal; ++i) senddata[i] = 1 + myid * numproc + i;

  std::cout << "Before ID=" << myid << "/" << numproc << ": ";
  for (int i = 0; i < ntotal; ++i) std::cout << senddata[i] << " ";
  std::cout << std::endl;

  // Do the operation. All nodes sends/receives data from all other nodes
  MPI_Alltoall(senddata, ndata, MPI_INT, recvdata, ndata, MPI_INT, 
	       MPI_COMM_WORLD);

  std::cout << "After  ID=" << myid << "/" << numproc << ": ";
  for (int i = 0; i < ntotal; ++i) std::cout << recvdata[i] << " ";
  std::cout << std::endl;

  MPI_Finalize();
  delete[] senddata;
  delete[] recvdata;
}
