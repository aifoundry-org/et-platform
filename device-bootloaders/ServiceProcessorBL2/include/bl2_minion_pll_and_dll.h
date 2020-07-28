#ifndef __MINION_PLL_AND_DLL_H__
#define __MINION_PLL_AND_DLL_H__

#include <stdint.h>

int configure_minion_plls_and_dlls(uint64_t shire_mask);
int enable_minion_neighborhoods(uint64_t shire_mask);
int enable_minion_threads(uint64_t shire_mask);

#endif
