#include <cstdint>
#include <iostream>
#include <vector>
#include <omp.h>

// Helper function for reporting current execution times.
// It prints the elapsed time between t1 and t2 under a descriptive label.
void timestamp(std::string label, double t1, double t2) {
  std::cout << label << " : " << t2 - t1 << " secs" << std::endl;
}

int main (int argc, char *argv[]) 
{
  const double t0 = omp_get_wtime();
  const uint64_t num_entries = 1UL << 30;
  std::cout << "Number of entries " << num_entries << std::endl;

  // Allocate three large vectors: x, y, and z.
  // Each element is a 64-bit integer, so this uses a lot of memory.
  // std::vector：C++ 标准库里的动态数组容器
  std::vector<uint64_t> x(num_entries);//创建一个长度为 num_entries 的数组
  std::vector<uint64_t> y(num_entries);
  std::vector<uint64_t> z(num_entries);

  const double t1 = omp_get_wtime();
  timestamp("alloc", t0, t1);

  // Initialize x and y in parallel.
  // Each thread writes different indices, so there is no data race.
#pragma omp parallel for
  for (uint64_t i = 0; i < num_entries; i++) {
    x[i] = i;
    y[i] = num_entries - i;
  }
  const double t2 = omp_get_wtime();
  timestamp("init", t1, t2);

  // Sequential computation for comparison.
  // This establishes a baseline time for performing z = x + y without OpenMP.
  for (uint64_t i = 0; i < num_entries; i++){
    z[i] = x[i] + y[i];
  }
  const double t3 = omp_get_wtime();
  timestamp("add_seq", t2, t3);

  // Compute z = x + y in parallel.
  // Each loop iteration is independent, so the workload can be divided across threads.
#pragma omp parallel for
  for (uint64_t i = 0; i < num_entries; i++)
    z[i] = x[i]+y[i];
  const double t4 = omp_get_wtime();
  timestamp("add_par", t3, t4);

  // Validate correctness.
  // Because x[i] + y[i] should equal num_entries for every i,
  // we check that z[i] - num_entries is zero for all elements.
#pragma omp parallel for
  for (uint64_t i = 0; i < num_entries; i++)
    if(z[i]-num_entries != 0) {
      // This is a lightweight debugging log; printing every mismatch can be expensive.
#pragma omp critical
      std::cout << "error at position " << i << std::endl;
    }

  const double t5 = omp_get_wtime();
  timestamp("check", t4, t5);

  std::cout << "Vector addition check complete." << std::endl;
}
