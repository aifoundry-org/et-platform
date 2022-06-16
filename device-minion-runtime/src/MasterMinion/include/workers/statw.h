/***********************************************************************
*
* Copyright (C) 2022 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
/***********************************************************************/
/*! \file statw.h
    \brief A C header that defines the device statistics sampler
    worker's interface.
*/
/***********************************************************************/

#ifndef _STATW_
#define _STATW_

typedef uint8_t statw_resource_type_e;

enum statw_resource_type {
    STATW_RESOURCE_DMA_READ,
    STATW_RESOURCE_DMA_WRITE,
    STATW_RESOURCE_CM,
    STATW_RESOURCE_DDR_READ,
    STATW_RESOURCE_DDR_WRITE,
    STATW_RESOURCE_L2_L3_READ,
    STATW_RESOURCE_L2_L3_WRITE,
};

/*! \def STATW_SAMPLING_INTERVAL
    \brief Minion frequency on silicon
    TODO: Get it from SP and make it available to all MM workers. Preferably it should be accessible without atomic operation.
*/
#define STATW_MINION_FREQ 333000000ULL

/*! \def STATW_SAMPLING_INTERVAL
    \brief Device statistics sampling interval of 100 millisecond
    WARNING: Assumption is timer granularity is one millisecond
*/
#define STATW_SAMPLING_INTERVAL 1U

/*! \def STATW_CMA_SAMPLE_COUNT
    \brief Device statistics moving average sample count.
*/
#define STATW_CMA_SAMPLE_COUNT 100U

/*! \def STATW_RESOURCE_DEFAULT_MIN
    \brief Default minimum for device resource stat. This is minimum value will be replaced by very first data sample.
*/
#define STATW_RESOURCE_DEFAULT_MIN 0

/*! \def STATW_RESOURCE_DEFAULT_MAX
    \brief Default maximum for device resource stat. This is minimum value will replace with actual maximum.
*/
#define STATW_RESOURCE_DEFAULT_MAX 0UL

/*! \def STATW_RESOURCE_DEFAULT_AVG
    \brief Default average for device resource stat.
*/
#define STATW_RESOURCE_DEFAULT_AVG 0UL

/*! \def STATW_NUM_OF_BYTES_IN_1MB
    \brief A macro that define number of bytes in 1 megabytes.
*/
#define STATW_NUM_OF_BYTES_IN_1MB 0x100000UL

/*! \def STATW_NUM_OF_MS_IN_SEC
    \brief A macro that define number of milliseconds in 1 second.
*/
#define STATW_NUM_OF_MS_IN_SEC 1000U

/*! \def MAX(x,y)
    \brief Returns max
*/
#define MAX(x, y) (x > y ? x : y)

/*! \def MIN(x,y)
    \brief Returns min
*/
#define MIN(x, y) (y == 0 ? x : x < y ? x : y)

/*! \fn void STATW_Launch(uint32_t sqw_idx)
    \brief Initialize Device Stat Workers, used by dispatcher
    \param hart_id HART ID on which the Stat Worker should be launched
    \return none
*/
void STATW_Launch(uint32_t hart_id);

/*! \fn uint32_t STATW_Add_New_Sample_Atomically(statw_resource_type_e resource_type, uint64_t current_sample)
    \brief This functions adds new sample for resource utilization data.
    \param resource_type Resource for which new sample is to be added.
    \param current_sample Sample data
    \return None.
*/
void STATW_Add_New_Sample_Atomically(statw_resource_type_e resource_type, uint64_t current_sample);

#endif