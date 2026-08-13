#include <iostream>
#include <omp.h>

int main (int argc, char *argv[]) 
{
  omp_set_num_threads(4);
  
  for(int i = 0; i < 5; ++i) {

    // Parallelize only when i==2    
#pragma omp parallel if(i==2)
    {
      // Output from Thread
#pragma omp critical
      std::cout << "Visit " << i << " tid " << omp_get_thread_num() << std::endl;
    }
    
    // Output Newline
    //std::cout << std::endl;
  }
}

