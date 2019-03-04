#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "interrupt_device.h"

#include <stdint.h>

void INT_enableInterrupt(interrupt_t interrupt, uint32_t priority, void (*function)(void));
void INT_disableInterrupt(interrupt_t interrupt);

#endif
