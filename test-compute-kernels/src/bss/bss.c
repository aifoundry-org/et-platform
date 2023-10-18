#include "etsoc/isa/hart.h"

static inline int min(int a, int b) { return a < b ? a : b; }

typedef struct {
  int* a;
  int* b;
  int* result;
  int numElements;
} MyVectors;

// int global[4000];
volatile int global[1000];

// use only one shire to exec this simple test
int entry_point(const MyVectors* const vectors) {
  int tid = (int)get_thread_id();
  if (tid == 0) return 0;
  int mid = (int)get_minion_id();
  int numWorkers = SOC_MINIONS_PER_SHIRE;
  int elemsPerWorker = (vectors->numElements + numWorkers - 1) / numWorkers;
  if (elemsPerWorker % 16) {
    elemsPerWorker += 16 - (elemsPerWorker % 16);
  }
  int init = elemsPerWorker * mid;
  int end = min(elemsPerWorker * (mid + 1), vectors->numElements);

  for (int i = init; i < end; ++i) {
    if (i == 123) {
      global[i] = 1;
    }
    vectors->result[i] = vectors->a[i] + vectors->b[i] + global[i % 1000];
  }
  return 0;
}
