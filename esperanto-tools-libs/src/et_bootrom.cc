#include "et_bootrom.h"

#include "et_device.h"

static unsigned char *gEtBootrom = nullptr;
static size_t bootrom_file_size = 0;


EXAPI unsigned char** etrtGetEtBootrom() {
    return &gEtBootrom;
}

EXAPI size_t* etrtGetEtBootromSize() {
    return &bootrom_file_size;
}

EXAPI void etrtKillDevice() {
	std::cout << "etrt Killing device\n";
    GetDev dev;
    dev.destroy();
    dev.~GetDev();
}
