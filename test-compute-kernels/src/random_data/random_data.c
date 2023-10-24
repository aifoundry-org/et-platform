#include <stddef.h>
#include <stdint.h>

#include "etsoc/isa/hart.h"
#include "lfsr.h"

typedef struct {
  uint64_t* data_ptr;
  uint64_t length;
  uint64_t lfsr;
} Parameters;

int64_t entry_point(const Parameters*);

int64_t entry_point(const Parameters* const kernel_params_ptr) {
  if (kernel_params_ptr == NULL || kernel_params_ptr->data_ptr == NULL ||
      kernel_params_ptr->length % 8 != 0 || kernel_params_ptr->lfsr == 0) {
    // Bad arguments
    return -1;
  }

  // Only the first HART in each shire does any work.
  // Probably only want to run this on one shire.
  if (get_hart_id() % 64 != 0) {
    // Nothing to do
    return 0;
  }

  uint64_t* data_ptr = (void*)kernel_params_ptr->data_ptr;
  uint64_t length = kernel_params_ptr->length;
  uint64_t lfsr = kernel_params_ptr->lfsr;

  while (length) {
    *data_ptr++ = update_lfsr(lfsr);
    length -= 8;
  }

  return 0;
}
