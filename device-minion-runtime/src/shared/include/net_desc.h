#ifndef NET_DESC_H
#define NET_DESC_H

#include <stdint.h>

typedef struct{
  uint64_t compute_pc;
  uint64_t helper_pc;
  uint64_t shire_mask;
  uint64_t minion_mask;
  uint64_t id;
  uint64_t tensor_a;
  uint64_t tensor_b;
  uint64_t tensor_c;
  uint64_t tensor_d;
  uint64_t tensor_e;
  uint64_t compute_size;
  uint64_t helper_size;
  uint64_t info_pointer;
} net_desc_t;

#endif
