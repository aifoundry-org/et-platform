/***********************************************************************
*
* Copyright (C) 2023 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
/***********************************************************************/
/*! \file abi.h
    \brief A C header that defines the ET-SOC ABI related data structures.
*/
/***********************************************************************/
#ifndef _ABI_H_
#define _ABI_H_

#include <stdint.h>

#ifndef __ASSEMBLER__
#include <assert.h>
#endif /* __ASSEMBLER__ */

/*! \def ABI_VERSION_MAJOR
    \brief This is ABI layout version (major).
*/
#define ABI_VERSION_MAJOR 0

/*! \def ABI_VERSION_MINOR
    \brief This is ABI layout version (minor).
*/
#define ABI_VERSION_MINOR 1

/*! \def ABI_VERSION_PATCH
    \brief This is ABI layout version (patch).
*/
#define ABI_VERSION_PATCH 0

/*! \typedef abi_version_t
    \brief Structure holding the ABI version fields.
*/
typedef struct {
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
    uint16_t reserved;
} __attribute__((packed)) abi_version_t;

/*! \typedef kernel_environment_t
    \brief Properties related to a kernel runtime environment.
    \warning This structure consumes a whole cache-line.
    Beware of the space (64 bytes) it occupies.
*/
typedef struct {
    abi_version_t version; /* Version of the ABI */
    uint64_t shire_mask;   /* Representing all the Compute Shires assigned to the current Kernel */
    uint32_t frequency;    /* Frequency of Minion cores in MHz */
} __attribute__((packed, aligned(64))) kernel_environment_t;

/************************/
/* Compile-time checks  */
/************************/
#ifndef __ASSEMBLER__

/* Ensure the kernel environment attributes are occupying a single cache-line. */
static_assert(sizeof(kernel_environment_t) == 64,
    "kernel_environment_t size must be equal to cache-line size (64 bytes)");

#endif /* __ASSEMBLER__ */

#endif /* _ABI_H_ */
