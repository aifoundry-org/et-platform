#ifndef PLIC_H
#define PLIC_H

#include <stdint.h>

#define PLIC_TARGETS 3
#define PLIC_SOURCES 256
#define PLIC_SOURCE_REGS ((PLIC_SOURCES + 31)  / 32) // for regs that have 32 sources each

typedef struct PLIC_priority_s {
    union {
        uint32_t R;
        struct {
            uint32_t priority:3;
            uint32_t unused:29;
        } B;
    };
} PLIC_priority_t;

typedef struct PLIC_pending_s {
    union {
        uint32_t R;
        struct {
            uint32_t status:32;
        } B;
    };
} PLIC_pending_t;

typedef struct PLIC_enable_s {
    union {
        uint32_t R;
        struct {
            uint32_t enable:32;
        } B;
    };
} PLIC_enable_t;

typedef struct PLIC_threshold_s {
    union {
        uint32_t R;
        struct {
            uint32_t threshold:3;
            uint32_t unused:29;
        } B;
    };
} PLIC_threshold_t;

typedef struct PLIC_maxID_s {
    union {
        uint32_t R;
        struct {
            uint32_t maxid:32;
        } B;
    };
} PLIC_maxID_t;

typedef struct PLIC_thresholdAndMaxID_s
{
    PLIC_threshold_t threhsold;
    PLIC_maxID_t maxID;
} PLIC_thresholdAndMaxID_t;

typedef struct PLIC_s {
    PLIC_priority_t priority[PLIC_SOURCES]; // 0x0
    PLIC_pending_t pending[PLIC_SOURCE_REGS]; // 0x1000
    PLIC_enable_t enable[PLIC_TARGETS][PLIC_SOURCE_REGS]; // 0x2000
    uint8_t padding[0x200000 - 0x2000];
    PLIC_thresholdAndMaxID_t thresholdAndMaxID[PLIC_TARGETS]; // 0x200000
} PLIC_t;

#endif // PLIC_H