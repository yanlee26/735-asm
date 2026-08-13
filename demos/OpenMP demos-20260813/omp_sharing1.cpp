/*
  Implicit sharing and privatizing of variabless
  
  To compile:
  g++ -fopenmp -o omp_sharing1 omp_sharing1.cpp
  
 */
#include <iostream>
#include <omp.h>

int main (int argc, char *argv[]) 
{
  // Implicitly shared
  int total = 0;

#pragma omp parallel
  {
    // Implicitly private
    int subtotal = 0;
    
#pragma omp for
    for(int i = 0; i < 1000; i++) {
      subtotal += i;
    }
    
#pragma omp atomic
    total += subtotal;
  }
  
  std::cout << "Total: " << total << std::endl;

}
