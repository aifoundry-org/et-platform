#include "kernel_params.h"
#include "shire.h"

typedef struct {
  int* a;
  int value;
  int numElements;
  int numShires;
} Parameters;

int main(const kernel_params_t* const kernel_params_ptr) {
  Parameters* params = (Parameters*)kernel_params_ptr->tensor_a;
  int hart = (int)get_hart_id();
  int numWorkers = params->numShires * SOC_MINIONS_PER_SHIRE * 2;
  for (int i = hart; i < params->numElements; i += numWorkers) {
    params->a[i] = params->value;
  }
  return 0;
}
