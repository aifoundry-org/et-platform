/* --------------------------------------------------------------------
** 
** Synopsys DesignWare AMBA Software Driver Kit and
** documentation (hereinafter, "Software") is an Unsupported
** proprietary work of Synopsys, Inc. unless otherwise expressly
** agreed to in writing between Synopsys and you.
** 
** The Software IS NOT an item of Licensed Software or Licensed
** Product under any End User Software License Agreement or Agreement
** for Licensed Product with Synopsys or any supplement thereto. You
** are permitted to use and redistribute this Software in source and
** binary forms, with or without modification, provided that
** redistributions of source code must retain this notice. You may not
** view, use, disclose, copy or distribute this file or any information
** contained herein except pursuant to this license grant from Synopsys.
** If you do not agree with this notice, including the disclaimer
** below, then you are not authorized to use the Software.
** 
** THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS"
** BASIS AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
** FOR A PARTICULAR PURPOSE ARE HEREBY DISCLAIMED. IN NO EVENT SHALL
** SYNOPSYS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
** EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
** OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
** USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
** DAMAGE.
** 
** --------------------------------------------------------------------
*/

#ifndef DW_COMMON_IO_H
#define DW_COMMON_IO_H

#ifdef __cplusplus
extern "C" {    // allow C++ to use these headers
#endif

// The following macros all perform 32-bit reads/writes for varying data
// bus widths.  For example, DW_OUT32_8 writes a 32-bit word in 8-bit
// chunks.
#define DW_OUT32_32P(v,p)   DW_OUT32_32(v, &(p))
#define DW_OUT32_16P(v,p)   DW_OUT32_16(v, &(p))
#define DW_OUT32_8P(v,p)    DW_OUT32_8(v, &(p))

#define DW_IN32_32P(p)      DW_IN32_32(&(p))
#define DW_IN32_16P(p)      DW_IN32_16(&(p))
#define DW_IN32_8P(p)       DW_IN32_8(&(p))

// The following macros perform 8-, 16- and 32-bit read/writes for
// various data bus widths.  These macros rely on a 'dev' structure of
// type dw_device, which has a 'data_width' member to determine the data
// bus width.
#define DW_OUT32P(v,p)      DW_OUT32(v, &(p))
#define DW_OUT16P(v,p)      DW_OUT16(v, &(p))
#define DW_OUT8P(v,p)       DW_OUT8(v, &(p))

#define DW_IN32P(p)         DW_IN32(&(p))
#define DW_IN16P(p)         DW_IN16(&(p))
#define DW_IN8P(p)          DW_IN8(&(p))

// The following macros perform 32-bit read/writes.  These macros rely
// on a 'dev' structure of type dw_device, which has a 'data_width'
// member to determine the data bus width.  They always read/write a
// 32-bit word, taking the data bus width into account.
#define DW_OUTP(v,p)        DW_OUT(v, &(p))
#define DW_INP(p)           DW_IN(&(p))

// The default behaviour is to check individual devices' bus data
// widths.  This assumes the presence of a 'dev' structure of type
// dw_device which has a 'data_width' member.  If this member contains
// an illegal value, reads/writes default to 8-bit accesses.
//
// If all device data bus widths are the same (i.e. a single APB bus),
// and APB_DATA_WIDTH is defined, we can define all the IN/OUT macros
// for the correct data width.
#ifdef DW_IO_STATIC
    
#if (APB_DATA_WIDTH == 32)

// 32-bit data bus.  All macros can be defined to use their respective
// data read/write widths.
#define DW_OUT(v,p)         DW_OUT32_32(v,p)
#define DW_IN(p)            DW_IN32_32(p)
#define DW_OUT32(v,p)       DW_OUT32_32(v,p)
#define DW_OUT16(v,p)       DW_OUT16_16(v,p)
#define DW_OUT8(v,p)        DW_OUT8_8(v,p)
#define DW_IN32(p)          DW_IN32_32(p)
#define DW_IN16(p)          DW_IN16_16(p)
#define DW_IN8(p)           DW_IN8_8(p)
    
#elif (APB_DATA_WIDTH == 16)

// 16-bit data bus.  The maximum allowable data write is limited to 16
// bits.  32-bit accesses must be performed in two 16-bit chunks.
#define DW_OUT(v,p)         DW_OUT32_16(v,p)
#define DW_IN(p)            DW_IN32_16(p)
#define DW_OUT32(v,p)       DW_OUT32_16(v,p)
#define DW_OUT16(v,p)       DW_OUT16_16(v,p)
#define DW_OUT8(v,p)        DW_OUT8_8(v,p)
#define DW_IN32(p)          DW_IN32_16(p)
#define DW_IN16(p)          DW_IN16_16(p)
#define DW_IN8(p)           DW_IN8_8(p)

#else

// The default is 8-bit reads/writes.  All accesses are performed one
// bytes at a time.
#define DW_OUT(v,p)         DW_OUT32_8(v,p)
#define DW_IN(p)            DW_IN32_8(p)
#define DW_OUT32(v,p)       DW_OUT32_8(v,p)
#define DW_OUT16(v,p)       DW_OUT16_8(v,p)
#define DW_OUT8(v,p)        DW_OUT8_8(v,p)
#define DW_IN32(p)          DW_IN32_8(p)
#define DW_IN16(p)          DW_IN16_8(p)
#define DW_IN8(p)           DW_IN8_8(p)

#endif      // APB_DATA_WIDTH

#else       // DW_IO_STATIC

// These macros assume a "struct dw_device *dev" variable within the
// current scope and should normally only be used by device driver code.
// The default is 8-bit wide reads/writes.
#define DW_OUT(v,p)                 \
do {                                \
    if(dev->data_width == 32)       \
        DW_OUT32_32(v,p);           \
    else if(dev->data_width == 16)  \
        DW_OUT32_16(v,p);           \
    else                            \
        DW_OUT32_8(v,p);            \
} while(0)

// default is 8-bit wide reads
#define DW_IN(p)                    \
    ((dev->data_width == 32) ?      \
     DW_IN32_32(p) :                \
    ((dev->data_width == 16) ?      \
     DW_IN32_16(p) :                \
     DW_IN32_8(p)))

#define DW_IN8(p)       DW_IN8_8(p)
#define DW_OUT8(v,p)    DW_OUT8_8(v,p)

#define DW_IN16(p)                  \
    ((dev->data_width == 8) ?       \
     DW_IN16_8(p) :                 \
     DW_IN16_16(p))

#define DW_OUT16(v, p)              \
do {                                \
    if(dev->data_width == 8)        \
        DW_OUT16_8(v,p);            \
    else                            \
        DW_OUT16_16(v,p);           \
} while(0)

#define DW_IN32(p)                  \
    ((dev->data_width == 32) ?      \
     DW_IN32_32(p) :                \
    ((dev->data_width == 16) ?      \
     DW_IN32_16(p) :                \
     DW_IN32_8(p)))

#define DW_OUT32(v,p)               \
do {                                \
    if(dev->data_width == 32)       \
        DW_OUT32_32(v,p);           \
    else if(dev->data_width == 16)  \
        DW_OUT32_16(v,p);           \
    else                            \
        DW_OUT32_8(v,p);            \
} while(0)

#endif      // DW_IO_STATIC

// The default endianness is little endian.
// Srdjan: commented out "if" part since we cannot find where BIG_ENDIAN
//         is defined and it is reading from memory wrong
/*#if defined(BIG_ENDIAN)

// The following macro performs a 32-bit write
#define DW_OUT32_32(v,p)                        \
do {                                            \
    uint32_t v32;                               \
    v32 = (v & 0x000000ff);                     \
    v32 <<= 8;                                  \
    v32 |= ((v & 0x0000ff00) >> 8);             \
    v32 <<= 8;                                  \
    v32 |= ((v & 0x00ff0000) >> 16);            \
    v32 <<= 8;                                  \
    v32 |= ((v & 0xff000000) >> 24);            \
    *((volatile uint32_t *) p) = v32;           \
} while(0)

// the following macro performs two 16-bit writes
#define DW_OUT32_16(v,p)                        \
do {                                            \
    volatile uint16_t *ptr16;                   \
    uint16_t v16;                               \
    ptr16 = (uint16_t *) p;                     \
    v16 = (uint16_t) ((v & 0xff000000) >> 24);  \
    v16 |= (uint16_t) ((v & 0x00ff0000) >> 8);  \
    *ptr16 = v16;                               \
    v16 = (uint16_t) ((v & 0x0000ff00) >> 8);   \
    v16 |= (uint16_t) ((v & 0x000000ff) << 8);  \
    *(ptr16 + 1) = v16;                         \
} while(0)

// the following macro performs four 8-bit writes
#define DW_OUT32_8(v,p)                         \
do {                                            \
    volatile uint8_t *ptr8;                     \
    uint8_t v8;                                 \
    ptr8 = (uint8_t *) p;                       \
    v8 = (uint8_t) ((v & 0xff000000) >> 24);    \
    *ptr8 = v8;                                 \
    v8 = (uint8_t) ((v & 0x00ff0000) >> 16);    \
    *(ptr8 + 1) = v8;                           \
    v8 = (uint8_t) ((v & 0x0000ff00) >> 8);     \
    *(ptr8 + 2) = v8;                           \
    v8 = (uint8_t) (v & 0x000000ff);            \
    *(ptr8 + 3) = v8;                           \
} while(0)

// the following macro performs a 16-bit write
#define DW_OUT16_16(v,p)                        \
do {                                            \
    uint16_t v16;                               \
    v16 = (uint16_t) ((v & 0xff00) >> 8);       \
    v16 |= (uint16_t) ((v & 0x00ff) << 8);      \
    *((volatile uint16_t *) p) = v16;           \
} while(0)

// the following macro performs two 8-bit writes
#define DW_OUT16_8(v,p)                         \
do {                                            \
    volatile uint8_t *ptr8;                     \
    uint16_t v8;                                \
    ptr8 = (uint8_t *) (p);                     \
    v8 = (uint8_t) (((v) & 0xff00) >> 8);       \
    *ptr8 = v8;                                 \
    v8 = (uint8_t) ((v) & 0x00ff);              \
    *(ptr8 + 1) = v8;                           \
} while(0)

// the following macro performs an 8-bit write
#define DW_OUT8_8(v,p)                              \
do {                                                \
    *((volatile uint8_t *) (p)) = (uint8_t) (v);    \
} while(0)

// the following macro performs a 32-bit read
#define DW_IN32_32(p)                               \
    ((uint32_t) (*((volatile uint8_t *) (p)) << 24) \
        | (*((volatile uint8_t *) (p) + 1) << 16)   \
        | (*((volatile uint8_t *) (p) + 2) << 8)    \
        | (*((volatile uint8_t *) (p) + 3)))

// the following macro performs two 16-bit reads
#define DW_IN32_16(p)                                           \
    ((uint32_t) ((*((volatile uint16_t *) (p)) & 0xff) << 24)   \
        | ((*((uint16_t *) (p)) & 0xff00) << 8)                 \
        | ((*((volatile uint16_t *) (p) + 1) & 0x00ff) << 8)    \
        | ((*((uint16_t *) (p) + 1) & 0xff00) >> 8))

// the following macro performs four 8-bit reads
#define DW_IN32_8(p)                                \
    ((uint32_t) (*((volatile uint8_t *) (p)) << 24) \
        | (*((volatile uint8_t *) (p) + 1) << 16)   \
        | (*((volatile uint8_t *) (p) + 2) << 8)    \
        | (*((volatile uint8_t *) (p) + 3)))

// the following macro performs a 16-bit read
#define DW_IN16_16(p)                               \
     ((uint16_t) (*((volatile uint8_t *) (p)) << 8) \
        | (*((volatile uint8_t *) (p) + 1)))

// the following macro performs two 8-bit reads
#define DW_IN16_8(p)                                \
     ((uint16_t) (*((volatile uint8_t *) (p)) << 8) \
        | (*((volatile uint8_t *) (p) + 1)))

// the following macro performs an 8-bit read
#define DW_IN8_8(p)                            \
     ((uint8_t) *((volatile uint8_t *) (p)))
*/
//#else   // default: little endian

// the following macro performs a 32-bit write
#define DW_OUT32_32(v,p)                        \
do {                                            \
    *((volatile uint32_t *) (p)) = (unsigned)(v);         \
} while(0)

// the following macro performs two 16-bit writes
#define DW_OUT32_16(v,p)                        \
do {                                            \
    volatile uint16_t *ptr16;                   \
    uint16_t v16;                               \
    ptr16 = (volatile uint16_t *) (p);                   \
    v16 = (uint16_t) ((v) & 0x0000ffff);        \
    *ptr16 = v16;                               \
    v16 = (uint16_t) (((v) & 0xffff0000) >> 16);\
    *(ptr16 + 1) = v16;                         \
} while (0)

// the following macro performs two 8-bit writes
#define DW_OUT32_8(v,p)                         \
do {                                            \
    volatile uint8_t *ptr8;                     \
    uint8_t v8;                                 \
    ptr8 = (volatile uint8_t *) (p);                     \
    v8 = (uint8_t) ((v) & 0x000000ff);          \
    *ptr8 = v8;                                 \
    v8 = (uint8_t) (((v) & 0x0000ff00) >> 8);   \
    *(ptr8 + 1) = v8;                           \
    v8 = (uint8_t) (((v) & 0x00ff0000) >> 16);  \
    *(ptr8 + 2) = v8;                           \
    v8 = (uint8_t) (((v) & 0xff000000) >> 24);  \
    *(ptr8 + 3) = v8;                           \
} while(0)

// the following macro performs 16-bit writes
#define DW_OUT16_16(v,p)                            \
do {                                                \
    *((volatile uint16_t *) (p)) = (uint16_t) (v);  \
} while(0)

// the following macro performs two 8-bit writes
#define DW_OUT16_8(v,p)                             \
do {                                                \
    volatile uint8_t *ptr8;                         \
    uint16_t v8;                                    \
    ptr8 = (uint8_t *) (p);                         \
    v8 = (uint8_t) ((v) & 0x00ff);                  \
    *ptr8 = v8;                                     \
    v8 = (uint8_t) (((v) & 0xff00) >> 8);           \
    *(ptr8 + 1) = v8;                               \
} while(0)

// the following macro performs an 8-bit write
#define DW_OUT8_8(v,p)                              \
do {                                                \
    *((volatile uint8_t *) (p)) = (uint8_t) (v);    \
} while(0)

// the following macro performs a 32-bit reads
#define DW_IN32_32(p)   *((volatile uint32_t *) (p))

// the following macro performs two 16-bit reads
#define DW_IN32_16(p)   ((uint32_t) *((volatile uint16_t *) (p))     \
        | (uint32_t) (*((volatile uint16_t *) (p) + 0x1u) << 16))

// the following macro performs 8-bit reads
#define DW_IN32_8(p)    ((uint32_t) ((*((volatile uint8_t *) (p)))    \
        | (*((volatile uint8_t *) (p) + 1) << 8)                     \
        | (*((volatile uint8_t *) (p) + 2) << 16)                    \
        | (*((volatile uint8_t *) (p) + 3) << 24)))

// the following macro performs a 16-bit read
#define DW_IN16_16(p)   ((uint16_t) *((volatile uint16_t *) (p)))

// the following macro performs two 8-bit reads
#define DW_IN16_8(p)    ((uint16_t) (*((volatile uint8_t *) (p))    \
    | (*((volatile uint8_t *) (p) + 1) << 8)))

// the following macro performs an 8-bit read
#define DW_IN8_8(p)     ((uint8_t) *((volatile uint8_t *) (p)))

//#endif  // defined(BIG_ENDIAN)


#ifdef __cplusplus
}
#endif

#endif  // DW_COMMON_IO_H

