#include "interrupt.h"

static void dummyIsr(void);

void (*vectorTable[PU_PLIC_INTR_CNT])(void) =
{
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr
};

void* pullVectorTable = vectorTable;

static void dummyIsr(void)
{

}
