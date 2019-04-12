#ifndef ET_BOOTROM_H
#define ET_BOOTROM_H

#include <et-misc.h>
#include <stdio.h>

EXAPI unsigned char **etrtGetEtBootrom();
EXAPI size_t *etrtGetEtBootromSize();

#endif // ET_BOOTROM_H