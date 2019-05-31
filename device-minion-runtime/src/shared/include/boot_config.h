#ifndef __BOOT_CONFIG_H__
#define __BOOT_CONFIG_H__

#include <stdint.h>

typedef union CONFIG_COMMAND_u {
    union {
        struct {
            uint32_t offset : 24;
            uint32_t memSpace : 4;
            uint32_t opCode : 4;
        } B;
        uint32_t R;
    } dw0;
    union {
        struct {
            uint32_t value : 32;
        } B;
        uint32_t R;
    } dw1;
    union {
        struct {
            uint32_t mask : 32;
        } B;
        uint32_t R;
    } dw2;
} CONFIG_COMMAND_t;

#endif

