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
  int N = std::rand();// 0-RAND_MAX, RAND_MAX = 2147483647 in my system, so N is a random number between 0 and 2^31-1. This is used to simulate a workload.
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


/*
...
Task: 58 1.36133 secs thread 5/10 (2020739063 1.01037e+09)
Task: 9 1.1833 secs thread 0/10 (1777724115 8.88862e+08)
Task: 16 0.961156 secs thread 1/10 (1478446501 7.39223e+08)
Task: 27 0.886495 secs thread 2/10 (1351934195 6.75967e+08)
Task: 38 1.19948 secs thread 3/10 (1864546517 9.32273e+08)
Task: 17 0.336878 secs thread 1/10 (500782188 2.50391e+08)
Task: 28 0.41777 secs thread 2/10 (657821123 3.28911e+08)
Task: 98 1.02345 secs thread 9/10 (1581030105 7.90515e+08)
Task: 67 1.0049 secs thread 6/10 (1557810404 7.78905e+08)
Task: 39 0.5052 secs thread 3/10 (753799505 3.769e+08)
Task: 18 0.682625 secs thread 1/10 (1102246882 5.51123e+08)
Task: 78 1.35738 secs thread 7/10 (2146319451 1.07316e+09)
Task: 59 1.21677 secs thread 5/10 (1908194298 9.54097e+08)
Task: 68 0.543951 secs thread 6/10 (884936716 4.42468e+08)
Task: 29 0.790219 secs thread 2/10 (1269406752 6.34703e+08)
Task: 79 0.32644 secs thread 7/10 (578354438 2.89177e+08)
Task: 99 1.03649 secs thread 9/10 (1816731566 9.08366e+08)
Task: 69 0.475793 secs thread 6/10 (892053144 4.46027e+08)
Task: 19 0.966584 secs thread 1/10 (1807130337 9.03565e+08)
*/