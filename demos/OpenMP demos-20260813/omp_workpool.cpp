#include <cstdint>
#include <iostream>
#include <vector>
#include <omp.h>

void timestamp(std::string label, double t1, double t2) {
  std::cout << label << " : " << t2 - t1 << " secs" << std::endl;
}

// A contrived example of a task
void do_task(int i) {

  const int tid = omp_get_thread_num();
  const int nthreads = omp_get_num_threads();

  
  double t0 = omp_get_wtime();
  int N = std::rand();
  double sum = 0;
  for (int n = 0; n < N; ++n) {
    double x = static_cast<double>(n) / N;
    sum += x;
  }
  double t1 = omp_get_wtime();

#pragma omp critical
  std::cout << "Task: " << i << " "
	    << t1 - t0 << " secs"
	    << " thread " << tid << "/" << nthreads << " ("
	    << N << " " << sum << ")" << std::endl;
}

int main (int argc, char *argv[]) 
{
#pragma omp parallel for
  for (int k = 0; k < 100; ++k)
    do_task(k);


  // Question: is the thread destroyed at the end of each task? No!
}
