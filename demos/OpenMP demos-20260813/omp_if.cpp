#include <iostream>
#include <omp.h>

/*
  omp_if.cpp
  Simple OpenMP example demonstrating the `if` clause on a parallel region.

  Behavior:
  - The loop runs from i=0..4.
  - The `#pragma omp parallel if(i==2)` creates a parallel region only when i==2.
    For other values of i the region executes serially (no thread team is spawned).
  - `omp_set_num_threads(4)` requests up to 4 threads for parallel regions.
  - The `#pragma omp critical` ensures the output from different threads
    does not interleave on the console.

  Compile with an OpenMP-capable compiler, e.g.:
    clang++ -fopenmp omp_if.cpp -o omp_if
    g++ -fopenmp omp_if.cpp -o omp_if
*/

int main(int argc, char *argv[])
{
  // Request 4 threads for parallel regions (scheduler may choose fewer)
  omp_set_num_threads(4);

  // Iterate five times; only iteration i==2 will run in parallel
  for (int i = 0; i < 5; ++i)
  {

    // Parallelize only when i==2
    // 有 if(i==2)）——只有当 i==2 时才并行，其他迭代由单个线程执行
    // 在你这个示例里，if(i==2) 是为了仅在第 2 次迭代利用多线程并行，去掉它会使每次迭代都并行（更多输出、更高开销）。需要根据工作量与线程开销权衡是否保留。
#pragma omp parallel if (i == 2)
    {
      // Each thread prints its visit and thread id.
      // The critical region serializes access to std::cout to avoid
      // interleaved output from multiple threads.
#pragma omp critical
      std::cout << "Visit " << i << " tid " << omp_get_thread_num() << std::endl;
    }

    // Optional: print a blank line between iterations (disabled)
    // std::cout << std::endl;
  }

  return 0;
}

/*
Visit 0 tid 0
Visit 1 tid 0
Visit 2 tid 0
Visit 2 tid 1
Visit 2 tid 3
Visit 2 tid 2
Visit 3 tid 0
Visit 4 tid 0

--------------
rm: #pragma omp parallel if(i==2)
Visit 0 tid 0
Visit 1 tid 0
Visit 2 tid 0
Visit 3 tid 0
Visit 4 tid 0

*/