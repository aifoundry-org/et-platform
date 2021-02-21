
#include "hart.h"
#include "atomic.h"
#include "common.h"
#include "log.h"
#include "flb.h"

#include <stdint.h>
#include <stddef.h>

#define BASE_ADDR_FOR_THIS_TEST  0x8200000000ULL

// The test takes tensor_a as a parameter
// This test takes an address offset as a parameter and adds it to the base address
// All the threads access the same address

typedef struct {
  uint64_t addr;
} Parameters;
int64_t main(const Parameters* const kernel_params_ptr) {
  if ((kernel_params_ptr == NULL)) {
    // Bad arguments
    log_write(LOG_LEVEL_CRITICAL, "Programming returing due to error\n");
    return -1;
  }

  // const int64_t hart_id = get_hart_id();
  uint64_t addr = kernel_params_ptr->addr & 0xFFF0;  // address offset
  long unsigned int shire_addr;
  volatile uint64_t* atomic_addr;
  shire_addr = BASE_ADDR_FOR_THIS_TEST | addr;
  atomic_addr = (uint64_t*)shire_addr;

  // Each minion updates the same addr
  atomic_add_global_64(atomic_addr, 0x1);
  return 0;
}
