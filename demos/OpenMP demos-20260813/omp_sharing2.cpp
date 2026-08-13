/*
  Explicit sharing and privatizing of variabless
  
  To compile:
  g++ -fopenmp -o omp_sharing2 omp_sharing2.cpp
  
 */
#include <iostream>
#include <omp.h>

int main (int argc, char *argv[]) 
{
  int total = 0;
  int subtotal = 0;

  // Explicitly declare the shared and private variables
#pragma omp parallel shared(total) private(subtotal)
  {
    subtotal = 0;

#pragma omp for
    for(int i = 0; i < 1000; i++) {
      subtotal += i;
    }
    
#pragma omp atomic
    total += subtotal;
  }
  
  std::cout << "Total: " << total << std::endl;

}
