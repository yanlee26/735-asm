/*
  159.735 C++ implemenation of broadcast example

  This also implements the partial sum problem from second.c/cpp and
  introduces the Reduce operation.

  Ian Bond
  Updated: 10/7/2025
 */
#include <cstdlib>
#include <iostream>
#include "mpi.h"

int main(int argc, char *argv[])
{
  // Initialize
  MPI_Init(&argc, &argv);

  // Workpool information
  int numproc, myid, namelen;
  char processor_name[MPI_MAX_PROCESSOR_NAME];
  MPI_Comm_size(MPI_COMM_WORLD, &numproc);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);
  MPI_Get_processor_name(processor_name, &namelen);

  if (argc<2) {
    std::cout << "No argument" << std::endl;
    MPI_Finalize();
    exit(0);
  }

  // If master process, get N from user input
  int N = 0;
  if (myid == 0) N = std::atoi(argv[1]);
  std::cout << "id=" << myid << " N=" << N << " before broadcast" << std::endl;

  // Broadcast from master to all slaves. If this is the master
  // prcess, then this routine is sending, if this is a slave process,
  // then the routine is receiving
  MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);
  std::cout << "id=" << myid << " N=" << N << " after broadcast" << std::endl;

  // Note here we don't need separate branching for master/slave operations
  int first = N / numproc * myid + 1;
  int last  = N / numproc * (myid + 1);
  int sum = 0;
  for (int i = first; i <= last; ++i) sum += i;

  // Combine the sums computed on each node, into result. If this is
  // one of the slaves, this routine will send sum to the master. If
  // this is the master process, then it will receive the sums and
  // combine them.
  int result;
  std::cout << "id=" << myid
	    << " before reduce: sum=" << sum
	    << " result=" << result << std::endl;
  MPI_Reduce(&sum, &result, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
  std::cout << "id=" << myid
	    << " after reduce: sum=" << sum
	    << " result=" << result << std::endl;

  if (myid == 0) std::cout << "Sum= " << result << std::endl;

  MPI_Finalize();
}
