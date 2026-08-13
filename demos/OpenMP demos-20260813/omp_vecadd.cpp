#include <cstdint>
#include <iostream>
#include <vector>
#include <omp.h>

// Helper function for reporting current execution times
void timestamp(std::string label, double t1, double t2) {
  std::cout << label << " : " << t2 - t1 << " secs" << std::endl;
}

int main (int argc, char *argv[]) 
{
  const double t0 = omp_get_wtime();
  const uint64_t num_entries = 1UL << 30;
  std::cout << "Number of entries " << num_entries << std::endl;
  
  std::vector<uint64_t> x(num_entries);
  std::vector<uint64_t> y(num_entries);
  std::vector<uint64_t> z(num_entries);

  const double t1 = omp_get_wtime();
  timestamp("alloc", t0, t1);

  // Initialization
#pragma omp parallel for
  for (uint64_t i = 0; i < num_entries; i++) {
    x[i] = i;
    y[i] = num_entries - i;
  }
  const double t2 = omp_get_wtime();
  timestamp("init", t1, t2);

  // Compute z = x + y sequentially
  for (uint64_t i = 0; i < num_entries; i++)
    z[i] = x[i] + y[i];
  const double t3 = omp_get_wtime();
  timestamp("add_seq", t2, t3);

  // Compute z = x + y in parallel
#pragma omp parallel for
  for (uint64_t i = 0; i < num_entries; i++)
    z[i] = x[i]+y[i];
  const double t4 = omp_get_wtime();
  timestamp("add_par", t3, t4);

  // Check if summation is correct
#pragma omp parallel for
  for (uint64_t i = 0; i < num_entries; i++)
    if(z[i]-num_entries != 0)
      std::cout << "error at position "
		<< i << std::endl;

  const double t5 = omp_get_wtime();
  timestamp("check", t4, t5);
  
}
