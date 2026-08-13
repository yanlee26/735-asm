/*
  159.735 MPI demo: scatter collective communication pattern

  Ian Bond
  Updated: 10/7/2025
 */
#include <cstdlib>
#include <iostream>
#include "mpi.h"

int main(int argc, char *argv[])
{
  // Number of data words per scatter
  const int ndata = 5;

  // Data will be "scattered" from this node to the others
  const int root = 0;

  MPI_Init(&argc, &argv);

  int numproc, myid;
  MPI_Comm_size(MPI_COMM_WORLD, &numproc);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);

  // All nodes must have enough space to receive the data
  int* sendbuf = new int[ndata * numproc];
  int* recvbuf = new int[ndata];

  // Fill up the array with data to send to the destination nodes
  if (myid == root) {
    std::cout << "SEND " << myid << " : ";
    for (int i = 0; i < ndata * numproc; ++i) {
      sendbuf[i] = i;
      std::cout << sendbuf[i] << " ";
    }
    std::cout << std::endl;
  }

  MPI_Scatter(sendbuf, ndata, MPI_INT, recvbuf, ndata, MPI_INT, root,
	      MPI_COMM_WORLD);

  std::cout << "RECV " << myid << " : ";
  for (int i = 0; i < ndata; ++i) {
    std::cout << recvbuf[i] << " ";
  }
  std::cout << std::endl;

  MPI_Finalize();

  delete[] sendbuf;
  delete[] recvbuf;
}
