#include <stdint.h>
#include <etsoc/isa/esr_defines.h>
#include <etsoc/isa/hart.h>

/* Defines for dummy MPROT in U-mode */
#define ESR_NEIGH_DUMMY_MPROT_REGNO  0x4
#define ESR_NEIGH_DUMMY_MPROT_PROT   PRV_U
#define ESR_NEIGH_DUMMY_MPROT_ACCESS esr_access_RW

/* Generates bus error */
int main(void)
{
    /* Generate bus error on single hart */
    if (get_hart_id() == 0)
    {
        /* Access a non-existent ESR. According to spec, it would generate a bus error */
        volatile uint64_t dummy = *((volatile uint64_t *)ESR_NEIGH(THIS_SHIRE, 0, DUMMY_MPROT));
    }

    return 0;
}
