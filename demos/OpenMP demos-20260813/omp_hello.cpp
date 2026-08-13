/*
  g++ -fopenmp -o omp_hello omp_hello.cpp
 */
#include <iostream>
#include <omp.h>

int main (int argc, char *argv[]) 
{
  const int nrequired = std::atoi(argv[1]);
  omp_set_num_threads(nrequired);
  
#pragma omp parallel
{
  const int tid = omp_get_thread_num();
  const int nthreads = omp_get_num_threads();
  const int num_proc = omp_get_num_procs();

#pragma omp critical
  std::cout << "Hello from thread " << tid 
	    << " of " << nthreads << " threads"
	    << " on " << num_proc << " processors" << std::endl;

}

}
