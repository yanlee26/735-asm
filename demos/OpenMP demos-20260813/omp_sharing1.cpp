/*
  Implicit sharing and privatizing of variabless

  To compile:
  g++ -fopenmp -o omp_sharing1 omp_sharing1.cpp

 */
#include <iostream>
#include <omp.h>

int main(int argc, char *argv[])
{
  // Implicitly shared
  int total = 0;

#pragma omp parallel
  {
    // Implicitly private
    int subtotal = 0;

#pragma omp for
    for (int i = 0; i < 1000; i++)
    {
      subtotal += i;

      // Log which thread performed this addition. This prints a lot
      // of lines for large iteration counts; remove or throttle in
      // production runs if not needed.
#pragma omp critical
      std::cout << "Thread " << omp_get_thread_num() << " added " << i << std::endl;
    }

#pragma omp atomic
    total += subtotal;
  }

  std::cout << "Total: " << total << std::endl;
}
