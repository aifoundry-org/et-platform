#include "shire.h"
struct Parameters {
  int* a;
  int value;
  int numElements;
};
inline int min(int a, int b) { return a < b ? a : b; }
// use only one shire to exec this test
int main(struct Parameters* params) {
  int tid = (int)get_thread_id();
  if (tid == 0) return 0;
  int mid = (int)get_minion_id();
  int numWorkers = SOC_MINIONS_PER_SHIRE;
  int elemsPerWorker = (params->numElements + numWorkers - 1) / numWorkers;
  int init = elemsPerWorker * mid;
  int end = min(elemsPerWorker * (mid + 1), params->numElements);
  for (int i = init; i < end; ++i) {
    params->a[i] = params->value;
  }
  return 0;
}
