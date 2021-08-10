#include "mock_etsoc.h"

uint64_t reg_mhartid = 0;
uint64_t reg_hpmcounter3 = 0;
uint64_t reg_hpmcounter4 = 0;
uint64_t reg_hpmcounter5 = 0;
uint64_t reg_hpmcounter6 = 0;
uint64_t reg_hpmcounter7 = 0;
uint64_t reg_hpmcounter8 = 0;

void etsoc_reset(void)
{
    reg_mhartid = 0;
    reg_hpmcounter3 = 0;
    reg_hpmcounter4 = 0;
    reg_hpmcounter5 = 0;
    reg_hpmcounter6 = 0;
    reg_hpmcounter7 = 0;
    reg_hpmcounter8 = 0;
}
