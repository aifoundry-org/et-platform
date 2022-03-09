#pragma once

#include <stdint.h>

/* ESR base addresses */
#define ESR_ABSCMD          0x01800007C0
#define ESR_NXPROGBUF0      0x01800007B0
#define ESR_NXPROGBUF1      0x01800007B8
#define ESR_AXPROGBUF0      0x01800007A0
#define ESR_AXPROGBUF1      0x01800007A8
#define ESR_NXDATA0         0x0180000780
#define ESR_NXDATA1         0x0180000788
#define ESR_AXDATA0         0x0180000790
#define ESR_AXDATA1         0x0180000798
#define ESR_HACTRL          0x018010FF80
#define ESR_HASTATUS0       0x018010FF88
#define ESR_HASTATUS1       0x018010FF90
#define ESR_ANDORTREEL0     0x018010FF98
#define ESR_ANDORTREEL1     0x018035ff80
#define ESR_DMCTRL          0x0052029008
#define ESR_ANDORTREEL2     0x005202900C
#define ESR_THREAD0_DISABLE 0x01C0340240
#define ESR_THREAD1_DISABLE 0x01C0340010


/* DMCTRL */
#define DMACTIVE        (1ull << 0)
#define NDMRESET        (1ull << 1)
#define CLRRESETHALTREQ (1ull << 2)
#define SETRESETHALTREQ (1ull << 3)
#define HASEL           (1ull << 26)
#define ACKHAVERESET    (1ull << 28)
#define HARTRESET       (1ull << 29)
#define RESUMEREQ       (1ull << 30)
#define HALTREQ         (1ull << 31)


/* ANDORTREEL2 */
#define ANYHALTED    0x7
#define ALLHALTED    (1ull << 3)
#define ANYRUNNING   (1ull << 4)
#define ALLRUNNING   (1ull << 5)
#define ANYRESUMEACK (1ull << 6)
#define ALLRESUMEACK (1ull << 7)
#define ANYHAVERESET (1ull << 8)
#define ALLHAVERESET (1ull << 9)


/* Helper macros to define ESR r/w functions */
#define MINION_ESR_RO(TYPE, NAME, BASE)                                \
    static inline TYPE read_##NAME(uint64_t shire, uint64_t hart)      \
    {                                                                  \
        return *(volatile TYPE*)(BASE + (shire << 22) + (hart << 12)); \
    }

#define MINION_ESR_RW(TYPE, NAME, BASE)                                       \
    MINION_ESR_RO(TYPE, NAME, BASE)                                           \
    static inline TYPE write_##NAME(uint64_t shire, uint64_t hart, TYPE data) \
    {                                                                         \
        *(volatile TYPE*)(BASE + (shire << 22) + (hart << 12)) = data;        \
    }

#define NEIGH_ESR_RO(TYPE, NAME, BASE)                                  \
    static inline TYPE read_##NAME(uint64_t shire, uint64_t neigh)      \
    {                                                                   \
        return *(volatile TYPE*)(BASE + (shire << 22) + (neigh << 16)); \
    }

#define NEIGH_ESR_RW(TYPE, NAME, BASE)                                         \
    NEIGH_ESR_RO(TYPE, NAME, BASE)                                             \
    static inline TYPE write_##NAME(uint64_t shire, uint64_t neigh, TYPE data) \
    {                                                                          \
        *(volatile TYPE*)(BASE + (shire << 22) + (neigh << 16)) = data;        \
    }

#define SHIRE_ESR_RO(TYPE, NAME, BASE) \
    static inline TYPE read_##NAME(uint64_t shire) { return *(volatile TYPE*)(BASE + (shire << 22)); }

#define SHIRE_ESR_RW(TYPE, NAME, BASE) \
    SHIRE_ESR_RO(TYPE, NAME, BASE)     \
    static inline TYPE write_##NAME(uint64_t shire, TYPE data) { *(volatile TYPE*)(BASE + (shire << 22)) = data; }

#define SYSTEM_ESR_RO(TYPE, NAME, BASE) \
    static inline TYPE read_##NAME(void) { return *(volatile TYPE*)(BASE); }

#define SYSTEM_ESR_RW(TYPE, NAME, BASE) \
    SYSTEM_ESR_RO(TYPE, NAME, BASE)     \
    static inline TYPE write_##NAME(TYPE data) { *(volatile TYPE*)(BASE) = data; }


/* Minion ESRs */
MINION_ESR_RW(uint64_t, abscmd, ESR_ABSCMD)
MINION_ESR_RW(uint64_t, nxprogbuf0, ESR_NXPROGBUF0)
MINION_ESR_RW(uint64_t, nxprogbuf1, ESR_NXPROGBUF1)
MINION_ESR_RW(uint64_t, axprogbuf0, ESR_AXPROGBUF0)
MINION_ESR_RW(uint64_t, axprogbuf1, ESR_AXPROGBUF1)
MINION_ESR_RW(uint64_t, nxdata0, ESR_NXDATA0)
MINION_ESR_RW(uint64_t, nxdata1, ESR_NXDATA1)
MINION_ESR_RW(uint64_t, axdata0, ESR_AXDATA0)
MINION_ESR_RW(uint64_t, axdata1, ESR_AXDATA1)

/* Neighborhood ESRs */
NEIGH_ESR_RW(uint64_t, hactrl, ESR_HACTRL)
NEIGH_ESR_RO(uint64_t, hastatus0, ESR_HASTATUS0)
NEIGH_ESR_RO(uint64_t, hastatus1, ESR_HASTATUS1)
NEIGH_ESR_RO(uint64_t, andortreel0, ESR_ANDORTREEL0)

/* Minion Shire ESRs */
SHIRE_ESR_RO(uint64_t, andortreel1, ESR_ANDORTREEL1)
SHIRE_ESR_RW(uint64_t, thread0_disable, ESR_THREAD0_DISABLE)
SHIRE_ESR_RW(uint64_t, thread1_disable, ESR_THREAD1_DISABLE)

/* IO Shire ESRs */
/* FIXME(cabul): make sure these are 32-bit for et-lib-debug! */
SYSTEM_ESR_RW(uint32_t, dmctrl, ESR_DMCTRL)
SYSTEM_ESR_RO(uint32_t, andortreel2, ESR_ANDORTREEL2)
