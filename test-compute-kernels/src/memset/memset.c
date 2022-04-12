
#include <etsoc/isa/hart.h>

typedef struct {
  int* a;
  int value;
  int numElements;
  int numShires;
} Parameters;

static inline int min(int a, int b) { return a < b ? a : b; }

int main(const Parameters* const params) {
  int hart = (int)get_hart_id();
  int numWorkers = params->numShires * SOC_MINIONS_PER_SHIRE * 2;

  int elemsPerWorker = (params->numElements + numWorkers - 1) / numWorkers;
  if (elemsPerWorker % 16) {
    elemsPerWorker += 16 - (elemsPerWorker % 16);
  }
  int init = elemsPerWorker * hart;
  int end = min(elemsPerWorker * (hart + 1), params->numElements);

  for (int i = init; i < end; ++i) {
    params->a[i] = params->value;
  }
  return 0;
}
