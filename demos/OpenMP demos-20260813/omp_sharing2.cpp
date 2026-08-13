/*
  Explicit sharing and privatizing of variabless
  
  To compile:
  g++ -fopenmp -o omp_sharing2 omp_sharing2.cpp
  
 */
#include <iostream>
#include <omp.h>

int main (int argc, char *argv[]) 
{
  // total is shared among all threads; all threads update it.
  // subtotal is private to each thread; each thread gets its own copy.
  int total = 0;
  int subtotal = 0;

  // Explicitly declare the shared and private variables.
  // `total` is visible to every thread, while each thread has its own `subtotal`.
#pragma omp parallel shared(total) private(subtotal)
  {
    subtotal = 0;

    // Each thread handles a different chunk of the loop.
    // `subtotal` is local to the thread, so there is no race condition here.
#pragma omp for
    for(int i = 0; i < 1000; i++) {
      subtotal += i;

      // Log which thread is doing the update for debugging.
      // This is intentionally inside the loop so you can observe thread activity.
#pragma omp critical
      std::cout << "Thread " << omp_get_thread_num()
                << " adds i=" << i << ", subtotal now=" << subtotal << std::endl;
    }

    // Add the per-thread subtotal into the shared total.
    // `atomic` prevents concurrent updates to `total` from different threads.
#pragma omp atomic
    total += subtotal;

#pragma omp critical
    std::cout << "Thread " << omp_get_thread_num()
              << " contributes subtotal=" << subtotal
              << " to shared total=" << total << std::endl;
  }

  std::cout << "Final total: " << total << std::endl;

}

/*
...
Thread 2 adds i=299, subtotal now=24950
Thread 1 adds i=199, subtotal now=14950
Thread 6 adds i=699, subtotal now=64950
Thread 0 adds i=99, subtotal now=4950
Thread 9 adds i=999, subtotal now=94950
Thread 4 adds i=499, subtotal now=44950
Thread 7 adds i=799, subtotal now=74950
Thread 0 contributes subtotal=4950 to shared total=4950
Thread 8 contributes subtotal=84950 to shared total=89900
Thread 3 contributes subtotal=34950 to shared total=259700
Thread 2 contributes subtotal=24950 to shared total=444550
Thread 1 contributes subtotal=14950 to shared total=499500
Thread 9 contributes subtotal=94950 to shared total=499500
Thread 4 contributes subtotal=44950 to shared total=499500
Thread 6 contributes subtotal=64950 to shared total=499500
Thread 7 contributes subtotal=74950 to shared total=499500
Thread 5 contributes subtotal=54950 to shared total=499500
Final total: 499500
*/