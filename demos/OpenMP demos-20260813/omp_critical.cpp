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
  // Number of threads requested by the user.
  const int nrequired = std::atoi(argv[1]);
  omp_set_num_threads(nrequired);

  // Shared string used to collect thread IDs.
  // This variable is shared by all threads, so it must be protected
  // when modified in parallel.
  std::string str;

#pragma omp parallel
{
  // Per-thread information.
  const int tid = omp_get_thread_num();
  const int nthreads = omp_get_num_threads();
  const int num_proc = omp_get_num_procs();
  /*
  In OpenMP, critical creates a mutually exclusive section:
only one thread can enter a given critical section at a time
other threads wait until it finishes
  */


  // critical(a): this critical section is named "a".
  // Only one thread can execute it at a time.
  // It serializes access to std::cout so that messages do not interleave.
#pragma omp critical(a)
  std::cout << "Hello from thread " << tid
            << " of " << nthreads << " threads"
            << " on " << num_proc << " processors" << std::endl;

  // critical(b): this is a different named critical section.
  // It is independent from critical(a), meaning threads can execute
  // critical(a) and critical(b) at the same time if they are in different
  // named critical regions.
  // Here, we use it to protect the shared string concatenation.
#pragma omp critical(b)
  str += std::to_string(tid) + " ";
}

  // After the parallel region ends, print the collected thread IDs.
  std::cout << "Thread ID list: " << str << std::endl;

}


/*
Hello from thread 0 of 8 threads on 10 processors
Hello from thread 2 of 8 threads on 10 processors
Hello from thread 4 of 8 threads on 10 processors
Hello from thread 1 of 8 threads on 10 processors
Hello from thread 7 of 8 threads on 10 processors
Hello from thread 3 of 8 threads on 10 processors
Hello from thread 6 of 8 threads on 10 processors
Hello from thread 5 of 8 threads on 10 processors
Thread ID list: 0 2 4 1 7 3 6 5 

- How can I understand the word critical here ?

“critical” in OpenMP means: “this part must be executed one-at-a-time”
The word is not a special math meaning here — it means:

“important / must be protected”
“serial section”
“mutual exclusion region”
In OpenMP, a critical region acts like a lock:

if thread A enters it, thread B must wait
once A finishes, B can enter
only one thread can be inside that named critical section at the same time
*/