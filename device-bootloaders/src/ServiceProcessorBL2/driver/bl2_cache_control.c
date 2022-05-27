/***********************************************************************
*
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
#include "bl2_cache_control.h"
#include <hwinc/etsoc_shire_cache_esr.h>
#include <hwinc/etsoc_shire_other_esr.h>
#include <hwinc/etsoc_neigh_esr.h>
#include "esr.h"

/*! \def KB_to_MB(size)
    \brief Converts kilobytes to megabytes.
*/
#define KB_to_MB(size) (size / 1024)

#define SRAM_SHIRE_CACHE_ERROR 0x1
#define SRAM_ICACHE_ERROR      0x2

static void sram_error_threshold_isr(uint8_t, uint8_t, uint64_t);
static void sram_error_uncorr_isr(uint8_t, uint8_t, uint64_t);
static void sram_error_handler(uint8_t minshire);
static void icache_error_handler(uint8_t minshire);

/* The driver can populate this structure with the defaults that will be used
   during the init phase.*/
static struct sram_and_icache_event_control_block event_control_block
    __attribute__((section(".data")));

static uint8_t get_highest_set_bit_offset(uint64_t shire_mask)
{
    return (uint8_t)(63 - __builtin_clzl(shire_mask));
}

int32_t icache_error_control_init(dm_event_isr_callback event_cb)
{
    uint64_t shire_mask;

    event_control_block.icache.ce_count = 0;
    event_control_block.icache.uce_count = 0;
    event_control_block.icache.ce_threshold = ICACHE_CORR_ERROR_THRESHOLD;
    event_control_block.icache.event_cb = event_cb;

    shire_mask = Minion_Get_Active_Compute_Minion_Mask();

    icache_enable_all_interrupts(shire_mask);

    return 0;
}

int32_t icache_error_control_deinit(uint64_t shire_mask)
{
    icache_disable_all_interrupts(shire_mask);
    event_control_block.icache.event_cb = NULL;

    return 0;
}

int32_t icache_err_interrupt_enable(uint64_t shire_mask, uint32_t interrupt_mask)
{
    uint8_t minshire;
    uint64_t icache_esr;

    /* mode    - machine (3)
   * shireID - for each
   * region  - neighborhood (0-3)
   * addres  - icache err ctl esr
   * bank    - not relevant
   */
    FOR_EACH_MINSHIRE(for (uint8_t neigh = 0; neigh < 4; neigh++) {
        icache_esr = read_esr_new(PP_MACHINE, minshire, REGION_NEIGHBOURHOOD, neigh,
                                  ETSOC_NEIGH_ESR_ICACHE_ERR_LOG_CTL_ADDRESS, 0);
        icache_esr = ETSOC_NEIGH_ESR_ICACHE_ERR_LOG_CTL_ERR_INTERRUPT_ENABLE_MODIFY(icache_esr,
                                                                                    interrupt_mask);
        write_esr_new(PP_MACHINE, minshire, REGION_NEIGHBOURHOOD, neigh,
                      ETSOC_NEIGH_ESR_ICACHE_ERR_LOG_CTL_ADDRESS, icache_esr, 0);
    })

    return 0;
}

int32_t icache_enable_ce_interrupt(uint64_t shire_mask)
{
    uint32_t interrupt_mask;

    interrupt_mask = BIT(SINGLE_BIT_ECC);
    icache_err_interrupt_enable(shire_mask, interrupt_mask);

    return 0;
}

int32_t icache_enable_uce_interrupt(uint64_t shire_mask)
{
    uint32_t interrupt_mask;

    interrupt_mask = BIT(DOUBLE_BIT_ECC);
    icache_err_interrupt_enable(shire_mask, interrupt_mask);

    return 0;
}

int32_t icache_enable_ecc_counter_saturation_interrupt(uint64_t shire_mask)
{
    uint32_t interrupt_mask;

    interrupt_mask = BIT(ECC_ERROR_COUNTER_SATURATION);
    icache_err_interrupt_enable(shire_mask, interrupt_mask);

    return 0;
}

int32_t icache_enable_all_interrupts(uint64_t shire_mask)
{
    uint32_t interrupt_mask;

    interrupt_mask = BIT(SINGLE_BIT_ECC) | BIT(DOUBLE_BIT_ECC) | BIT(ECC_ERROR_COUNTER_SATURATION);
    icache_err_interrupt_enable(shire_mask, interrupt_mask);

    return 0;
}

int32_t icache_err_interrupt_disable(uint64_t shire_mask, uint32_t interrupt_mask)
{
    uint8_t minshire;
    uint64_t icache_esr;

    /* mode    - machine (3)
   * shireID - for each
   * region  - neighborhood (0-3)
   * addres  - icache err ctl esr
   * bank    - not relevant
   */
    FOR_EACH_MINSHIRE(for (uint8_t neigh = 0; neigh < 4; neigh++) {
        icache_esr = read_esr_new(PP_MACHINE, minshire, REGION_NEIGHBOURHOOD, neigh,
                                  ETSOC_NEIGH_ESR_ICACHE_ERR_LOG_CTL_ADDRESS, 0);
        interrupt_mask = ~interrupt_mask &
                         ETSOC_NEIGH_ESR_ICACHE_ERR_LOG_CTL_ERR_INTERRUPT_ENABLE_GET(icache_esr);
        icache_esr = ETSOC_NEIGH_ESR_ICACHE_ERR_LOG_CTL_ERR_INTERRUPT_ENABLE_MODIFY(icache_esr,
                                                                                    interrupt_mask);
        write_esr_new(PP_MACHINE, minshire, REGION_NEIGHBOURHOOD, neigh,
                      ETSOC_NEIGH_ESR_ICACHE_ERR_LOG_CTL_ADDRESS, icache_esr, 0);
    })

    return 0;
}

int32_t icache_disable_ce_interrupt(uint64_t shire_mask)
{
    uint32_t interrupt_mask;

    interrupt_mask = BIT(SINGLE_BIT_ECC);
    icache_err_interrupt_disable(shire_mask, interrupt_mask);

    return 0;
}

int32_t icache_disable_uce_interrupt(uint64_t shire_mask)
{
    uint32_t interrupt_mask;

    interrupt_mask = BIT(DOUBLE_BIT_ECC);
    icache_err_interrupt_disable(shire_mask, interrupt_mask);

    return 0;
}

int32_t icache_disble_ecc_counter_saturation_interrupt(uint64_t shire_mask)
{
    uint32_t interrupt_mask;

    interrupt_mask = BIT(ECC_ERROR_COUNTER_SATURATION);
    icache_err_interrupt_disable(shire_mask, interrupt_mask);

    return 0;
}

int32_t icache_disable_all_interrupts(uint64_t shire_mask)
{
    uint32_t interrupt_mask;

    interrupt_mask = BIT(SINGLE_BIT_ECC) | BIT(DOUBLE_BIT_ECC) | BIT(ECC_ERROR_COUNTER_SATURATION);
    icache_err_interrupt_disable(shire_mask, interrupt_mask);

    return 0;
}

static void icache_error_handler(uint8_t minshire)
{
    uint8_t neigh;
    bool error_occured = false;
    uint64_t error_log_info;
    uint64_t error_code;
    uint8_t error_type;
    msg_id_t event_id;

    struct event_message_t message;
    uint64_t error_log_address;

    /* Extract error code from ERR_LOG_INFO ESR */
    for (neigh = 0; neigh < 4; neigh++)
    {
        error_log_info = read_esr_new(PP_MACHINE, minshire, REGION_NEIGHBOURHOOD, neigh,
                                      ETSOC_NEIGH_ESR_ICACHE_ERR_LOG_INFO_ADDRESS, 0);
        if (ETSOC_NEIGH_ESR_ICACHE_ERR_LOG_INFO_SBE_DBE_CODE_GET(error_log_info))
        {
            error_occured = true;
            break;
        }
    }

    /* Skip further steps if error not founded */
    if (error_occured != true)
        return;

    error_code = ETSOC_NEIGH_ESR_ICACHE_ERR_LOG_INFO_SBE_DBE_CODE_GET(error_log_info);

    /* increment counter and set error type */
    switch (error_code)
    {
        case SINGLE_BIT_ECC:
            event_control_block.icache.uce_count++;
            error_type = CORRECTABLE;
            break;
        case DOUBLE_BIT_ECC:
            event_control_block.icache.ce_count++;
            error_type = UNCORRECTABLE;
            break;
        case ECC_ERROR_COUNTER_SATURATION:
            event_control_block.icache.ce_count++;
            error_type = CORRECTABLE;
            break;
        default:
            Log_Write(LOG_LEVEL_CRITICAL, "Unexpected error code.");
            return;
    }

    /* prepare and send message in case od uncorrecatable message or correctable threshold
     is reached */
    if ((error_type == UNCORRECTABLE) ||
        (event_control_block.icache.ce_count >= event_control_block.icache.ce_threshold))
    {
        if (error_code != ECC_ERROR_COUNTER_SATURATION)
        {
            /* Read PA associated with this error */
            error_log_address = read_esr_new(PP_MACHINE, minshire, REGION_NEIGHBOURHOOD, neigh,
                                             ETSOC_NEIGH_ESR_ICACHE_ERR_LOG_ADDRESS_ADDRESS, 0);
        }
        else
        {
            error_log_address = read_esr_new(PP_MACHINE, minshire, REGION_NEIGHBOURHOOD, neigh,
                                             ETSOC_NEIGH_ESR_ICACHE_SBE_DBE_COUNTS_ADDRESS, 0);
        }

        /* Add info about minshire and type of error SHIRE_CACHE/ICACHE */
        error_log_address |= ((uint64_t)minshire << 56) | ((uint64_t)SRAM_ICACHE_ERROR << 48);

        if (error_type == UNCORRECTABLE)
        {
            event_id = SRAM_UCE;
        }
        else
        {
            event_id = SRAM_CE;
        }

        /* add details in message header and fill payload */
        FILL_EVENT_HEADER(&message.header, event_id, sizeof(struct event_message_t))
        FILL_EVENT_PAYLOAD(&message.payload, CRITICAL, event_control_block.icache.uce_count,
                           error_log_info, error_log_address)
        /* call the callback function and post message */
        event_control_block.sram.event_cb(error_type, &message);
    }

    /* clear valid bit */
    error_log_info = ETSOC_NEIGH_ESR_ICACHE_ERR_LOG_INFO_SBE_DBE_VALID_MODIFY(error_log_info, 1);
    write_esr_new(PP_MACHINE, minshire, REGION_NEIGHBOURHOOD, neigh,
                  ETSOC_NEIGH_ESR_ICACHE_ERR_LOG_INFO_ADDRESS, error_log_info, 0);
}

int32_t sram_error_control_init(dm_event_isr_callback event_cb)
{
    uint64_t shire_mask;

    event_control_block.sram.ce_count = 0;
    event_control_block.sram.uce_count = 0;
    event_control_block.sram.ce_threshold = SRAM_CORR_ERROR_THRESHOLD;
    event_control_block.sram.event_cb = event_cb;

    shire_mask = Minion_Get_Active_Compute_Minion_Mask();

    sram_enable_all_interrupts(shire_mask);

    return 0;
}

int32_t sram_error_control_deinit(uint64_t shire_mask)
{
    uint8_t minshire;

    sram_disable_all_interrupts(shire_mask);
    event_control_block.sram.event_cb = NULL;

    FOR_EACH_MINSHIRE(INT_disableInterrupt(SPIO_PLIC_MINSHIRE_ERR0_INTR + minshire);)

    return 0;
}

int32_t sc_err_interrupt_enable(uint64_t shire_mask, uint32_t interrupt_mask)
{
    uint64_t sc_esr;
    uint8_t minshire;

    FOR_EACH_MINSHIRE(for (uint8_t bank = 0; bank < 4; bank++) {
        sc_esr = read_esr_new(PP_MACHINE, minshire, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
                              ETSOC_SHIRE_CACHE_ESR_SC_ERR_LOG_CTL_ADDRESS, bank);
        sc_esr = ETSOC_SHIRE_CACHE_ESR_SC_ERR_LOG_CTL_ESR_SC_ERR_INTERRUPT_ENABLE_MODIFY(
            sc_esr, interrupt_mask);
        write_esr_new(PP_MACHINE, minshire, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
                      ETSOC_SHIRE_CACHE_ESR_SC_ERR_LOG_CTL_ADDRESS, sc_esr, bank);
    })

    return 0;
}

int32_t sram_enable_ce_interrupt(uint64_t shire_mask)
{
    uint32_t interrupt_mask;

    interrupt_mask = BIT(SINGLE_BIT_ECC);
    sc_err_interrupt_enable(shire_mask, interrupt_mask);

    return 0;
}

int32_t sram_enable_uce_interrupt(uint64_t shire_mask)
{
    uint32_t interrupt_mask;

    interrupt_mask = BIT(DOUBLE_BIT_ECC);
    sc_err_interrupt_enable(shire_mask, interrupt_mask);

    return 0;
}

int32_t sram_enable_ecc_counter_saturation_interrupt(uint64_t shire_mask)
{
    uint32_t interrupt_mask;

    interrupt_mask = BIT(ECC_ERROR_COUNTER_SATURATION);
    sc_err_interrupt_enable(shire_mask, interrupt_mask);

    return 0;
}

int32_t sram_enable_decode_and_slave_interrupt(uint64_t shire_mask)
{
    uint32_t interrupt_mask;

    interrupt_mask = BIT(DECODE_AND_SLAVE_ERRORS);
    sc_err_interrupt_enable(shire_mask, interrupt_mask);

    return 0;
}

int32_t sram_enable_perf_counter_saturation_interrupt(uint64_t shire_mask)
{
    uint32_t interrupt_mask;

    interrupt_mask = BIT(PERFORMANCE_COUNTER_SATURATION);
    sc_err_interrupt_enable(shire_mask, interrupt_mask);

    return 0;
}

int32_t sram_enable_all_interrupts(uint64_t shire_mask)
{
    uint32_t interrupt_mask;

    interrupt_mask = BIT(SINGLE_BIT_ECC) | BIT(DOUBLE_BIT_ECC) | BIT(ECC_ERROR_COUNTER_SATURATION) |
                     BIT(DECODE_AND_SLAVE_ERRORS) | BIT(PERFORMANCE_COUNTER_SATURATION);
    sc_err_interrupt_enable(shire_mask, interrupt_mask);

    return 0;
}

int32_t sc_err_interrupt_disable(uint64_t shire_mask, uint32_t interrupt_mask)
{
    uint64_t sc_esr;
    uint8_t minshire;

    FOR_EACH_MINSHIRE(for (uint8_t bank = 0; bank < 4; bank++) {
        sc_esr = read_esr_new(PP_MACHINE, minshire, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
                              ETSOC_SHIRE_CACHE_ESR_SC_ERR_LOG_CTL_ADDRESS, bank);
        interrupt_mask =
            ~interrupt_mask &
            ETSOC_SHIRE_CACHE_ESR_SC_ERR_LOG_CTL_ESR_SC_ERR_INTERRUPT_ENABLE_GET(sc_esr);
        sc_esr = ETSOC_SHIRE_CACHE_ESR_SC_ERR_LOG_CTL_ESR_SC_ERR_INTERRUPT_ENABLE_MODIFY(
            sc_esr, interrupt_mask);
        write_esr_new(PP_MACHINE, minshire, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
                      ETSOC_SHIRE_CACHE_ESR_SC_ERR_LOG_CTL_ADDRESS, sc_esr, bank);
    })
    return 0;
}

int32_t sram_disable_ce_interrupt(uint64_t shire_mask)
{
    uint32_t interrupt_mask;

    interrupt_mask = BIT(SINGLE_BIT_ECC);
    sc_err_interrupt_disable(shire_mask, interrupt_mask);

    return 0;
}

int32_t sram_disable_uce_interrupt(uint64_t shire_mask)
{
    uint32_t interrupt_mask;

    interrupt_mask = BIT(DOUBLE_BIT_ECC);
    sc_err_interrupt_disable(shire_mask, interrupt_mask);

    return 0;
}

int32_t sram_disable_all_interrupts(uint64_t shire_mask)
{
    uint32_t interrupt_mask;

    interrupt_mask = BIT(SINGLE_BIT_ECC) | BIT(DOUBLE_BIT_ECC) | BIT(ECC_ERROR_COUNTER_SATURATION) |
                     BIT(DECODE_AND_SLAVE_ERRORS) | BIT(PERFORMANCE_COUNTER_SATURATION);
    sc_err_interrupt_disable(shire_mask, interrupt_mask);

    return 0;
}

int32_t sram_set_ce_threshold(uint32_t ce_threshold)
{
    /* set countable errors threshold */
    event_control_block.sram.ce_threshold = ce_threshold;
    return 0;
}

int32_t sram_get_ce_count(uint32_t *ce_count)
{
    /* get correctable errors count */
    *ce_count = event_control_block.sram.ce_count;
    return 0;
}

int32_t sram_get_uce_count(uint32_t *uce_count)
{
    /* get un-correctable errors count */
    *uce_count = event_control_block.sram.uce_count;
    return 0;
}

static void sram_error_threshold_isr(uint8_t minshire, uint8_t bank, uint64_t error_log_info)
{
    if (++event_control_block.sram.ce_count > event_control_block.sram.ce_threshold)
    {
        struct event_message_t message;
        uint64_t error_log_address;

        /* Read PA associated with this error */
        error_log_address = read_esr_new(PP_MACHINE, minshire, REGION_OTHER,
                                         ESR_OTHER_SUBREGION_CACHE,
                                         ETSOC_SHIRE_CACHE_ESR_SC_ERR_LOG_ADDRESS_ADDRESS, bank);

        /* Add info about minshire and type of error SHIRE_CACHE/ICACHE */
        error_log_address |= ((uint64_t)minshire << 56) | ((uint64_t)SRAM_SHIRE_CACHE_ERROR << 48);

        /* add details in message header and fill payload */
        FILL_EVENT_HEADER(&message.header, SRAM_CE, sizeof(struct event_message_t))
        FILL_EVENT_PAYLOAD(&message.payload, CRITICAL, event_control_block.sram.ce_count,
                           error_log_info, error_log_address)

        /* call the callback function and post message */
        event_control_block.sram.event_cb(CORRECTABLE, &message);
    }
}

static void sram_error_uncorr_isr(uint8_t minshire, uint8_t bank, uint64_t error_log_info)
{
    struct event_message_t message;
    uint64_t error_log_address;

    event_control_block.sram.uce_count++;

    /* Read PA associated with this error */
    error_log_address = read_esr_new(PP_MACHINE, minshire, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
                                     ETSOC_SHIRE_CACHE_ESR_SC_ERR_LOG_ADDRESS_ADDRESS, bank);

    /* Add info about minshire and type of error SHIRE_CACHE/ICACHE */
    error_log_address |= ((uint64_t)minshire << 56) | ((uint64_t)SRAM_SHIRE_CACHE_ERROR << 48);

    /* add details in message header and fill payload */
    FILL_EVENT_HEADER(&message.header, SRAM_UCE, sizeof(struct event_message_t))
    FILL_EVENT_PAYLOAD(&message.payload, CRITICAL, event_control_block.sram.uce_count,
                       error_log_info, error_log_address)

    /* call the callback function and post message */
    event_control_block.sram.event_cb(UNCORRECTABLE, &message);
}

static void sram_error_handler(uint8_t minshire)
{
    uint8_t bank;
    bool error_occured = false;
    uint64_t error_log_info;
    uint64_t error_code;

    /* Extract error code from ERR_LOG_INFO ESR */
    for (bank = 0; bank < 4; bank++)
    {
        error_log_info = read_esr_new(PP_MACHINE, minshire, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
                                      ETSOC_SHIRE_CACHE_ESR_SC_ERR_LOG_INFO_ADDRESS, bank);
        if (ETSOC_SHIRE_CACHE_ESR_SC_ERR_LOG_INFO_ECC_SINGLE_DOUBLE_VALID_GET(error_log_info))
        {
            error_occured = true;
            break;
        }
    }

    /* Skip further steps if error not founded */
    if (error_occured != true)
        return;

    error_code = ETSOC_SHIRE_CACHE_ESR_SC_ERR_LOG_INFO_ECC_SINGLE_DOUBLE_CODE_GET(error_log_info);

    switch (error_code)
    {
        case SINGLE_BIT_ECC:
            sram_error_threshold_isr(minshire, bank, error_log_info);
            break;
        case DOUBLE_BIT_ECC:
        case ECC_ERROR_COUNTER_SATURATION:
        case DECODE_AND_SLAVE_ERRORS:
        case PERFORMANCE_COUNTER_SATURATION:
            sram_error_uncorr_isr(minshire, bank, error_log_info);
            break;
        default:
            Log_Write(LOG_LEVEL_CRITICAL, "Unexpected error code.");
            break;
    }

    /* clear valid bit */
    error_log_info =
        ETSOC_SHIRE_CACHE_ESR_SC_ERR_LOG_INFO_ECC_SINGLE_DOUBLE_VALID_MODIFY(error_log_info, 1);
    write_esr_new(PP_MACHINE, minshire, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
                  ETSOC_SHIRE_CACHE_ESR_SC_ERR_LOG_INFO_ADDRESS, error_log_info, bank);
}

void sram_and_icache_error_isr(void)
{
    uint8_t minshire = 0;
    uint32_t ulmaxid = ioread32(SPIO_PLIC + SPIO_PLIC_MAXID_T0_ADDRESS);

    if ((ulmaxid >= SPIO_PLIC_MINSHIRE_ERR0_INTR) && (ulmaxid <= SPIO_PLIC_MINSHIRE_ERR33_INTR))
    {
        minshire = (uint8_t)(ulmaxid - SPIO_PLIC_MINSHIRE_ERR0_INTR);
    }
    else
    {
        Log_Write(LOG_LEVEL_CRITICAL, "Wrong interrupt handler");
        return;
    }

    sram_error_handler(minshire);
    icache_error_handler(minshire);
}

uint16_t Cache_Control_SCP_size(uint64_t shire_mask)
{
    uint64_t scp_cache_ctrl;
    uint16_t bank_scp_size;
    uint8_t num_of_active_shires;

    /* All cache banks are configured the same accross the chip, so we are reading
       bank 0 scp cache control register just from one shire */
    scp_cache_ctrl = read_esr_new(PP_MACHINE, 0, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
                                  ETSOC_SHIRE_CACHE_ESR_SC_SCP_CACHE_CTL_ADDRESS, 0);

    bank_scp_size = ETSOC_SHIRE_CACHE_ESR_SC_SCP_CACHE_CTL_ESR_SC_SCP_SET_SIZE_GET(scp_cache_ctrl);

    num_of_active_shires = (uint8_t)__builtin_popcountll(shire_mask);

    return (uint16_t)KB_to_MB(num_of_active_shires * SC_BANK_NUM * bank_scp_size);
}

uint16_t Cache_Control_L2_size(uint64_t shire_mask)
{
    uint64_t l2_cache_ctrl;
    uint16_t bank_l2_size;
    uint8_t num_of_active_shires;

    /* All cache banks are configured the same accross the chip, so we are reading
       bank 0 l2 cache control register just from one shire */
    l2_cache_ctrl = read_esr_new(PP_MACHINE, 0, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
                                 ETSOC_SHIRE_CACHE_ESR_SC_L2_CACHE_CTL_ADDRESS, 0);

    bank_l2_size = ETSOC_SHIRE_CACHE_ESR_SC_L2_CACHE_CTL_ESR_SC_L2_SET_SIZE_GET(l2_cache_ctrl);

    num_of_active_shires = (uint8_t)__builtin_popcountll(shire_mask);

    return (uint16_t)KB_to_MB(num_of_active_shires * SC_BANK_NUM * bank_l2_size);
}

uint16_t Cache_Control_L3_size(uint64_t shire_mask)
{
    uint64_t l3_cache_ctrl;
    uint16_t bank_l3_size;
    uint8_t num_of_active_shires;

    /* All cache banks are configured the same accross the chip, so we are reading
       bank 0 l3 cache control register just from one shire */
    l3_cache_ctrl = read_esr_new(PP_MACHINE, 0, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
                                 ETSOC_SHIRE_CACHE_ESR_SC_L3_CACHE_CTL_ADDRESS, 0);

    bank_l3_size = ETSOC_SHIRE_CACHE_ESR_SC_L3_CACHE_CTL_ESR_SC_L3_SET_SIZE_GET(l3_cache_ctrl);

    num_of_active_shires = (uint8_t)__builtin_popcountll(shire_mask);

    return (uint16_t)KB_to_MB(num_of_active_shires * SC_BANK_NUM * bank_l3_size);
}

int cache_scp_l2_l3_size_config(uint16_t scp_size, uint16_t l2_size, uint16_t l3_size,
                                uint64_t shire_mask)
{
    uint64_t scp_ctl_value;
    uint64_t l2_ctl_value;
    uint64_t l3_ctl_value;
    uint8_t num_of_shires;
    uint16_t set_base;
    uint16_t set_size;
    uint16_t set_mask;
    uint16_t tag_mask;
    uint8_t high_bit;

    if ((uint32_t)(scp_size + l2_size + l3_size) > SC_BANK_SIZE)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if (0 != shire_mask)
    {
        num_of_shires = get_highest_set_bit_offset(shire_mask);
    }
    else
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if (((scp_size % 2) != 0) || ((l2_size % 2) != 0) || ((l3_size % 2) != 0))
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if (l2_size < 0x40u)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    /* SCP calculation */
    set_base = 0;
    set_size = scp_size;
    high_bit = get_highest_set_bit_offset((uint64_t)scp_size);
    set_mask = (uint16_t)((0x1u << high_bit) - 0x1u);
    tag_mask = ((0x1u << high_bit) == scp_size) ? (uint16_t)((0x1u << high_bit) - 0x1u) :
                                                  (uint16_t)((0x1u << (high_bit - 0x1u)) - 0x1u);

    scp_ctl_value = ETSOC_SHIRE_CACHE_ESR_SC_SCP_CACHE_CTL_ESR_SC_SCP_SET_BASE_SET(set_base) |
                    ETSOC_SHIRE_CACHE_ESR_SC_SCP_CACHE_CTL_ESR_SC_SCP_SET_SIZE_SET(set_size) |
                    ETSOC_SHIRE_CACHE_ESR_SC_SCP_CACHE_CTL_ESR_SC_SCP_SET_MASK_SET(set_mask) |
                    ETSOC_SHIRE_CACHE_ESR_SC_SCP_CACHE_CTL_ESR_SC_SCP_TAG_MASK_SET(tag_mask);

    /* L2 calculation */
    set_base = scp_size;
    set_size = l2_size;
    high_bit = get_highest_set_bit_offset((uint64_t)l2_size);
    set_mask = (uint16_t)((0x1u << high_bit) - 0x1u);
    tag_mask = ((0x1u << high_bit) == l2_size) ? (uint16_t)((0x1u << high_bit) - 0x1u) :
                                                 (uint16_t)((0x1u << (high_bit - 0x1u)) - 0x1u);

    l2_ctl_value = ETSOC_SHIRE_CACHE_ESR_SC_L2_CACHE_CTL_ESR_SC_L2_SET_BASE_SET(set_base) |
                   ETSOC_SHIRE_CACHE_ESR_SC_L2_CACHE_CTL_ESR_SC_L2_SET_SIZE_SET(set_size) |
                   ETSOC_SHIRE_CACHE_ESR_SC_L2_CACHE_CTL_ESR_SC_L2_SET_MASK_SET(set_mask) |
                   ETSOC_SHIRE_CACHE_ESR_SC_L2_CACHE_CTL_ESR_SC_L2_TAG_MASK_SET(tag_mask);

    /* L3 calculation */
    set_base = (uint16_t)(scp_size + l2_size);
    set_size = l3_size;
    high_bit = get_highest_set_bit_offset((uint64_t)l3_size);
    set_mask = (uint16_t)((0x1u << high_bit) - 0x1u);
    tag_mask = ((0x1u << high_bit) == l3_size) ? (uint16_t)((0x1u << high_bit) - 0x1u) :
                                                 (uint16_t)((0x1u << (high_bit - 0x1u)) - 0x1u);

    l3_ctl_value = ETSOC_SHIRE_CACHE_ESR_SC_L3_CACHE_CTL_ESR_SC_L3_SET_BASE_SET(set_base) |
                   ETSOC_SHIRE_CACHE_ESR_SC_L3_CACHE_CTL_ESR_SC_L3_SET_SIZE_SET(set_size) |
                   ETSOC_SHIRE_CACHE_ESR_SC_L3_CACHE_CTL_ESR_SC_L3_SET_MASK_SET(set_mask) |
                   ETSOC_SHIRE_CACHE_ESR_SC_L3_CACHE_CTL_ESR_SC_L3_TAG_MASK_SET(tag_mask);

    /* Set shire cache configuration */
    for (uint8_t i = 0; i <= num_of_shires; i++)
    {
        if (shire_mask & 1)
        {
            /* Set scp cache ctrl */
            write_esr_new(PP_MACHINE, i, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
                          ETSOC_SHIRE_CACHE_ESR_SC_SCP_CACHE_CTL_ADDRESS, scp_ctl_value,
                          SC_BANK_BROADCAST);

            /* Set l2 cache ctrl */
            write_esr_new(PP_MACHINE, i, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
                          ETSOC_SHIRE_CACHE_ESR_SC_L2_CACHE_CTL_ADDRESS, l2_ctl_value,
                          SC_BANK_BROADCAST);

            /* Set l3 cache ctrl */
            write_esr_new(PP_MACHINE, i, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
                          ETSOC_SHIRE_CACHE_ESR_SC_L3_CACHE_CTL_ADDRESS, l3_ctl_value,
                          SC_BANK_BROADCAST);
        }
        shire_mask >>= 1;
    }

    return 0;
}
