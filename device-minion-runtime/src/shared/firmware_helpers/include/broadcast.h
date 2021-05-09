#ifndef BROADCAST_H
#define BROADCAST_H

#include <stdint.h>

uint64_t broadcast_encode_parameters(uint64_t pp, uint64_t region, uint64_t address);
int64_t broadcast_with_parameters(uint64_t value, uint64_t shire_mask, uint64_t parameters);
int64_t broadcast(uint64_t value, uint64_t shire_mask, uint64_t priv, uint64_t region,
                  uint64_t address);

#endif
