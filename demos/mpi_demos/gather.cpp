#include <cstdlib>
/*
  159.735 MPI demo: gather collective communication pattern

  Ian Bond
  Updated: 10/7/2025
 */
#include <iostream>
#include "mpi.h"

int main(int argc, char *argv[])
{
  // Data from other nodes will be "gathered" to this node
  const int dest = 0;

  MPI_Init(&argc, &argv);

  int numproc, myid;
  MPI_Comm_size(MPI_COMM_WORLD, &numproc);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);

  // Fill up the array with data that will be gathered by the
  // destination node. Note that the contents of the array will depend
  // on the process ID
  int ndata = 3 * numproc;
  int* sendarray = new int[ndata];
  std::cout << "SEND " << myid << " : ";
  for (int i = 0; i < ndata; ++i) {
    sendarray[i] = 100 * myid + i;
    std::cout << sendarray[i] << " ";
  }
  std::cout << std::endl;

  // Need to allocate memory to receive the data - note that it needs
  // to be big enough to receive from all processors
  int* recvarray = new int[ndata * numproc];

  MPI_Gather(sendarray, ndata, MPI_INT, recvarray, ndata, MPI_INT, dest,
	     MPI_COMM_WORLD);

  // Look at what has been gathered - note the difference depending on
  // the node
  std::cout << "RECV " << myid << " : ";
  for (int i = 0; i < ndata * numproc; ++i) {
    std::cout << recvarray[i] << " ";
  }
  std::cout << std::endl;

  MPI_Finalize();

  delete[] sendarray;
  delete[] recvarray;
}
