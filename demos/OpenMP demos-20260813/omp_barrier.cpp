/*
  g++ -fopenmp -o omp_barrier omp_barrier.cpp
 */
#include <iostream>
#include <omp.h>

int main (int argc, char *argv[]) 
{

#pragma omp parallel num_threads(8)
  {
    // Get thread id
    int tid = omp_get_thread_num();
    
    // Print
#pragma omp critical
    std::cout << tid << " started." << std::endl;

    // All threads wait here before proceeding
#pragma omp barrier
    
    // Print
#pragma omp critical
    std::cout << tid << " finished." << std::endl;
  }
  

}
