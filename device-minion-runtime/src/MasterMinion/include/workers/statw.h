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

/*! \typedef resource_value
    \brief Device statistics sample structure
*/
typedef struct {
    uint64_t avg;
    uint64_t min;
    uint64_t max;
} __attribute__((packed, aligned(8))) resource_value;

/*! \typedef compute_resources_sample
    \brief Device computer resource structure
*/
typedef struct {
    resource_value cm_utilization;
    resource_value pcie_dma_read_bw;
    resource_value pcie_dma_write_bw;
    resource_value ddr_read_bw;
    resource_value ddr_write_bw;
    resource_value l2_l3_read_bw;
    resource_value l2_l3_write_bw;
} __attribute__((packed, aligned(8))) compute_resources_sample;

typedef uint8_t statw_resource_type_e;

enum statw_resource_type { STATW_RESOURCE_DMA_READ, STATW_RESOURCE_DMA_WRITE, STATW_RESOURCE_CM };

/*! \def STATW_SAMPLING_INTERVAL
    \brief Device statistics sampling interval
*/
#define STATW_SAMPLING_INTERVAL 500U

/*! \def STATW_CMA_SAMPLE_COUNT
    \brief Device statistics moving average sample count.
*/
#define STATW_CMA_SAMPLE_COUNT 30U

/*! \def STATW_CMA_SAMPLE_INDEX_START
    \brief Device statistics moving average starting index of array.
*/
#define STATW_CMA_SAMPLE_INDEX_START 0U

/*! \def STATW_CMA_SAMPLE_INDEX_END
    \brief Device statistics moving average last index of array.
*/
#define STATW_CMA_SAMPLE_INDEX_END (STATW_CMA_SAMPLE_COUNT - 1U)

/*! \def STATW_INIT_SAMPLE_INDEX
    \brief Index to initialize stat worker's sample array. This should only be used once for very first sample.
    \NOTE: This value should alway be greater than STATW_CMA_SAMPLE_INDEX_END.
*/
#define STATW_INIT_SAMPLE_INDEX 0xFFFFFFU

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

/*! \fn uint32_t STATW_Add_Resource_Utilization_Sample
        (statw_resource_type_e resource_type, uint64_t current_sample, uint32_t index)
    \brief This functions adds new sample for resource utilization data.
           It puts data into a circular buffer. It automatically overrides
           the oldest values in the buffer.
    \param resource_type Resource for which new sample is to be added.
    \param current_sample Sample data
    \param index Index of circular buffer.
    \return Next index of circular buffer.
*/
uint32_t STATW_Add_Resource_Utilization_Sample(
    statw_resource_type_e resource_type, uint64_t current_sample, uint32_t index);

#endif