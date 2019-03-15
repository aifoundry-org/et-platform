// Device-specific interrupt setings needed by the shared interrupt code

#ifndef INTERRUPT_DEVICE_H
#define INTERRUPT_DEVICE_H

#define PLIC_BASE_ADDRESS 0x0050000000ULL

typedef enum interrupt_e
{
    INTERRUPT_INT1 = 1,
    INTERRUPT_INT2 = 2,
    NUM_INTERRUPTS
} interrupt_t;

#endif
