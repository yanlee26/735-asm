/* 
  Sequential version of the bucket sort routine, ie.e non MPI version

  To compile on ctcp cluster 

  gcc -o bucket -lm bucket.c

*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*****************************************************************************/
// Routines used in the sequential implementation of the bucket sort
float* create_buckets(int nbuckets, int nitems);
void bucket_sort(float *data, int ndata, float x1, float x2, int nbuckets,
		 float* bucket);

int check(float *data,int nitems) {
  double sum=0;
  int sorted=1;
  int i;

  for(i=0;i<nitems;i++) {
     sum+=data[i];
     if(i && data[i]<data[i-1]) sorted=0;
  }
  printf("sum=%f, sorted=%d\n",sum,sorted);
}

int compare(const void* x1, const void* x2);
/*****************************************************************************/

int main(int argc,char *argv[]) {
  // Specify here the full range that the data numbers can cover. We
  // may assume that all the numbers are positive definite
  const float xmin = 10.0;
  const float xmax = 250000;
  int nbuckets=1000;
  int nitems=100000;
  int i;

  if(argc==2) nitems=atoi(argv[1]);

  float *data=malloc(nitems*sizeof(float));

  for(i=0;i<nitems;i++)
    data[i]=drand48()*(xmax-xmin-1)+xmin;

  check(data,nitems);

  float *buckets=create_buckets(nbuckets,nitems);
  bucket_sort(data,nitems,xmin,xmax,nbuckets,buckets);

  check(data,nitems);

}
/*****************************************************************************/

// Sequential implementation of the bucket sort routine. The full
// range x1 to x2 will be divided into a number of equally spaced
// subranges according to the number of buckets. All the buckets are
// contained in the single one dimensional array "bucket".
void bucket_sort(float *data, int ndata, float x1, float x2, int nbuckets,
		 float *bucket) 
{
  int i, count;

  // The range covered by one bucket
  float stepsize = (x2 - x1) / nbuckets;

  // The number of items thrown into each bucket. We would expect each
  // bucket to have a similar number of items, but they won't be
  // exactly the same. So we keep track of their numbers here.
  int* nitems = malloc(nbuckets * sizeof(int));
  for (i = 0; i < nbuckets; ++i) nitems[i] = 0;

  // Toss the data items into the correct bucket
  for (i = 0; i < ndata; ++i) {

    // What bucket does this data value belong to?
    int bktno = (int)floor((data[i] - x1) / stepsize);
    int idx = bktno * ndata + nitems[bktno];

    //printf("DATA %d %f %d %d\n", i, data[i], bktno, idx);

    // Put the data value into this bucket
    bucket[idx] = data[i];
    ++nitems[bktno];
  }
  
  // Sort each bucket using the standard library qsort routine. Note
  // that we need to input the correct number of items in each bucket
  count = 0;
  for (i = 0; i < nbuckets; ++i) {
    if(nitems[i]) {
      qsort(&bucket[i*ndata], nitems[i], sizeof(float), compare);
      memcpy(data,&bucket[i*ndata],nitems[i]*sizeof(float));
      data+=nitems[i];
    }
  }

  // Don't need the number of items anymore
  free(nitems);
  
}

/*****************************************************************************/

// Create a data array to hold the given number of buckets for the
// given number of total data items. All buckets are held contiguously
// in the
float* create_buckets(int nbuckets, int nitems)
{
  int i;

  int ntotal = nbuckets * nitems;

  // Pointer to an array of more pointers to each bucket
  float* bucket = calloc(ntotal, sizeof(float*));
  for (i=0; i<ntotal; ++i) bucket[i] = 0;

  // return the address of the array of pointers to float arrays
  return bucket;
}

/*****************************************************************************/

// The comparison function to use with the library qsort
// function. This will tell qsort to sort the numbers in ascending
// order.
int compare(const void* x1, const void* x2) {
  const float* f1 = x1;
  const float* f2 = x2;
  float diff = *f1 - *f2;

  return (diff < 0) ? -1 : 1;
}
