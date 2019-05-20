#ifndef IPI_H
#define IPI_H

#include <stdint.h>

// shire = shire to send the software interrupt to, 0-32 or 0xFF for "this shire"
// Bit 0 in the mask corresponds to hart 0 in the shire (minion 0, thread 0), bit 1 to hart 1 (minion 0, thread 1) and bit 63 corresponds to hart 63 (minion 31, thread 1).
#define IPI_TRIGGER(shire, bitmask) (*(volatile uint64_t* const)(0x01C0340090ULL | ((shire & 0xFFU) << 22U)) = bitmask)
#define IPI_TRIGGER_CLEAR(shire, bitmask) (*(volatile uint64_t* const)(0x01C0340098ULL | ((shire & 0xFFU) << 22U)) = bitmask)

#endif // IPI_H
