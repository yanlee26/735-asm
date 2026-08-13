/*
  To compile:
  g++ -fopenmp -o omp_critical omp_critical.cpp

  Usage:
  ./omp_critical [number of threads requested]

  E.g.
  ./omp_critical 12
  
 */
#include <iostream>
#include <omp.h>

int main (int argc, char *argv[]) 
{
  const int nrequired = std::atoi(argv[1]);
  omp_set_num_threads(nrequired);

  std::string str;
  
#pragma omp parallel
{
  const int tid = omp_get_thread_num();
  const int nthreads = omp_get_num_threads();
  const int num_proc = omp_get_num_procs();

#pragma omp critical(a)
  std::cout << "Hello from thread " << tid 
	    << " of " << nthreads << " threads"
	    << " on " << num_proc << " processors" << std::endl;


#pragma omp critical(b)
  str += std::to_string(tid) + " ";
}

 std::cout << "Thread ID list: " << str << std::endl;

}
