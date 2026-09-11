/*
  159.735 MPI demo: all_to_all collective communication pattern

  Ian Bond
  Updated: 10/7/2025

  This example demonstrates `MPI_Alltoall` where each process sends
  `ndata` words to every other process. The send buffer is laid out
  as a contiguous array of length `numproc * ndata`, organized as
  [to_rank0, to_rank1, ..., to_rankN-1] (each block of size `ndata`).

  After `MPI_Alltoall(sendbuf, ndata, ...)` the receive buffer contains
  data from every source process in the same per-rank order; when
  `ndata == 1`, `recvdata[j]` is the datum sent from rank `j`.
*/
#include <iostream>
#include "mpi.h"

int main(int argc, char *argv[])
{
  MPI_Init(&argc,&argv);

  int numproc, myid;
  MPI_Comm_size(MPI_COMM_WORLD, &numproc);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);

  // Number of data words sent to each peer (per-destination count)
  const int ndata = 1; // try >1 to see multi-word transfers

  // Total amount of data in the send/receive buffers
  const int ntotal = numproc * ndata;

  // Allocate send and receive buffers
  int* senddata = new int[ntotal];
  int* recvdata = new int[ntotal];

  // Fill up the sending data with values that identify sender and index
  for (int i = 0; i < ntotal; ++i) senddata[i] = 1 + myid * numproc + i;

  // Log: show what this rank will send (helps trace data movement)
  std::cout << "Rank " << myid << " sending (ndata=" << ndata << ", ntotal=" << ntotal << ") : ";
  for (int i = 0; i < ntotal; ++i) std::cout << senddata[i] << " ";
  std::cout << std::endl;

  // Do the operation. All nodes send/receive data from all other nodes.
  MPI_Alltoall(senddata, ndata, MPI_INT, recvdata, ndata, MPI_INT,
               MPI_COMM_WORLD);

  // Log: show what this rank received. When ndata==1, recvdata[j]
  // contains the single integer sent from rank j.
  std::cout << "Rank " << myid << " received: ";
  for (int i = 0; i < ntotal; ++i) std::cout << recvdata[i] << " ";
  std::cout << std::endl;

  MPI_Finalize();
  delete[] senddata;
  delete[] recvdata;
}

/*
Rank 0 sending (ndata=1, ntotal=4) : 1 2 3 4 
Rank 1 sending (ndata=1, ntotal=4) : 5 6 7 8 
Rank 2 sending (ndata=1, ntotal=4) : 9 10 11 12 
Rank 3 sending (ndata=1, ntotal=4) : 13 14 15 16 
Rank 0 received: 1 5 9 13 
Rank 1 received: 2 6 10 14 
Rank 2 received: 3 7 11 15 
Rank 3 received: 4 8 12 16 

------------

Rank 0 sending (ndata=2, ntotal=8) : 1 2 3 4 5 6 7 8 
Rank 1 sending (ndata=2, ntotal=8) : 5 6 7 8 9 10 11 12 
Rank 2 sending (ndata=2, ntotal=8) : 9 10 11 12 13 14 15 16 
Rank 3 sending (ndata=2, ntotal=8) : 13 14 15 16 17 18 19 20 
Rank 0 received: 1 2 5 6 9
Rank 1 received: 3 4 7 8 11 12 15 16 
Rank 2 received: 5 6 9 10 13 14 17 18 
Rank 3 received: 7 8 11 12 15 16 19 20 
 10 13 14 
*/ 