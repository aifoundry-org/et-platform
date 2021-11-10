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
#include "esr.h"

/*! \def KB_to_MB(size)
    \brief Converts kilobytes to megabytes.
*/
#define KB_to_MB(size) \
    (size / 1024)

static void sram_error_threshold_isr(uint8_t, uint8_t, uint64_t);
static void sram_error_uncorr_isr(uint8_t, uint8_t, uint64_t);
static void sram_error_isr(void);

/* The driver can populate this structure with the defaults that will be used
   during the init phase.*/
static struct sram_event_control_block event_control_block
    __attribute__((section(".data")));

static uint8_t get_highest_set_bit_offset(uint64_t shire_mask)
{
    return (uint8_t)(63 - __builtin_clzl(shire_mask));
}

int32_t sram_error_control_init(dm_event_isr_callback event_cb) {
  uint64_t shire_mask;

  event_control_block.ce_count = 0;
  event_control_block.uce_count = 0;
  event_control_block.ce_threshold = SRAM_CORR_ERROR_THRESHOLD;
  event_control_block.event_cb = event_cb;

  shire_mask = Minion_Get_Active_Compute_Minion_Mask();
  sram_enable_uce_interrupt(shire_mask);
  sram_enable_ce_interrupt(shire_mask);

  return 0;
}

int32_t sram_error_control_deinit(uint64_t shire_mask) {
  sram_disable_ce_interrupt(shire_mask);
  sram_disable_uce_interrupt(shire_mask);

  return 0;
}

int32_t sram_enable_ce_interrupt(uint64_t shire_mask) {
  uint8_t minshire;
  uint64_t sc_esr;

  FOR_EACH_MINSHIRE(
      INT_enableInterrupt(SPIO_PLIC_MINSHIRE_ERR0_INTR + minshire, 1,
                          sram_error_isr);
      for (uint8_t bank = 0; bank < 4; bank++ ) {
            sc_esr = read_esr_new(
                PP_MACHINE, minshire, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
                ETSOC_SHIRE_CACHE_ESR_SC_ERR_LOG_CTL_ADDRESS, bank);
            sc_esr =
                ETSOC_SHIRE_CACHE_ESR_SC_ERR_LOG_CTL_ESR_SC_ERR_INTERRUPT_ENABLE_MODIFY(
                sc_esr, 0x1);
            write_esr_new(PP_MACHINE, minshire, REGION_OTHER,
                    ESR_OTHER_SUBREGION_CACHE,
                    ETSOC_SHIRE_CACHE_ESR_SC_ERR_LOG_CTL_ADDRESS, sc_esr, bank);
      } )

  return 0;
}

int32_t sram_enable_uce_interrupt(uint64_t shire_mask) {
  uint8_t minshire;
  uint64_t sc_esr;

  FOR_EACH_MINSHIRE(
      INT_enableInterrupt(SPIO_PLIC_MINSHIRE_ERR0_INTR + minshire, 1,
                          sram_error_isr);
      for (uint8_t bank = 0; bank < 4; bank++) {
          sc_esr = read_esr_new(
              PP_MACHINE, minshire, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
              ETSOC_SHIRE_CACHE_ESR_SC_ERR_LOG_CTL_ADDRESS, bank);
          sc_esr =
              ETSOC_SHIRE_CACHE_ESR_SC_ERR_LOG_CTL_ESR_SC_ERR_INTERRUPT_ENABLE_MODIFY(
                  sc_esr, 0x2);
          write_esr_new(PP_MACHINE, minshire, REGION_OTHER,
                        ESR_OTHER_SUBREGION_CACHE,
                        ETSOC_SHIRE_CACHE_ESR_SC_ERR_LOG_CTL_ADDRESS, sc_esr, bank);
      })
  return 0;
}

int32_t sram_disable_ce_interrupt(uint64_t shire_mask) {
  uint8_t minshire;
  uint64_t sc_esr;

  FOR_EACH_MINSHIRE(
      for (uint8_t bank = 0; bank < 4; bank++) {
         sc_esr = read_esr_new(
             PP_MACHINE, minshire, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
             ETSOC_SHIRE_CACHE_ESR_SC_ERR_LOG_CTL_ADDRESS, bank);
         sc_esr =
             ETSOC_SHIRE_CACHE_ESR_SC_ERR_LOG_CTL_ESR_SC_ERR_INTERRUPT_ENABLE_MODIFY(
                 sc_esr, 0x0);
         write_esr_new(PP_MACHINE, minshire, REGION_OTHER,
                       ESR_OTHER_SUBREGION_CACHE,
                       ETSOC_SHIRE_CACHE_ESR_SC_ERR_LOG_CTL_ADDRESS, sc_esr, bank);
      })
  return 0;
}

int32_t sram_disable_uce_interrupt(uint64_t shire_mask) {
  uint8_t minshire;

  FOR_EACH_MINSHIRE(
      INT_disableInterrupt(SPIO_PLIC_MINSHIRE_ERR0_INTR + minshire);)
  return 0;
}

int32_t sram_set_ce_threshold(uint32_t ce_threshold) {
  /* set countable errors threshold */
  event_control_block.ce_threshold = ce_threshold;
  return 0;
}

int32_t sram_get_ce_count(uint32_t *ce_count) {
  /* get correctable errors count */
  *ce_count = event_control_block.ce_count;
  return 0;
}

int32_t sram_get_uce_count(uint32_t *uce_count) {
  /* get un-correctable errors count */
  *uce_count = event_control_block.uce_count;
  return 0;
}

static void sram_error_threshold_isr(uint8_t minshire, uint8_t bank,
                                     uint64_t error_log_info) {
  if (++event_control_block.ce_count > event_control_block.ce_threshold) {
    struct event_message_t message;
    uint64_t error_log_address;

    /* Read PA associated with this error */
    error_log_address = read_esr_new(
        PP_MACHINE, minshire, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
        ETSOC_SHIRE_CACHE_ESR_SC_ERR_LOG_ADDRESS_ADDRESS, bank);

    /* add details in message header and fill payload */
    FILL_EVENT_HEADER(&message.header, SRAM_CE, sizeof(struct event_message_t))
    FILL_EVENT_PAYLOAD(&message.payload, CRITICAL, event_control_block.ce_count,
                       error_log_info, error_log_address)

    /* call the callback function and post message */
    event_control_block.event_cb(CORRECTABLE, &message);
  }
}

static void sram_error_uncorr_isr(uint8_t minshire, uint8_t bank, uint64_t error_log_info) {
  struct event_message_t message;
  uint64_t error_log_address;

  event_control_block.uce_count++;

  /* Read PA associated with this error */
  error_log_address = read_esr_new(
      PP_MACHINE, minshire, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
      ETSOC_SHIRE_CACHE_ESR_SC_ERR_LOG_ADDRESS_ADDRESS, bank);

  /* add details in message header and fill payload */
  FILL_EVENT_HEADER(&message.header, SRAM_UCE, sizeof(struct event_message_t))
  FILL_EVENT_PAYLOAD(&message.payload, CRITICAL, event_control_block.uce_count,
                     error_log_info, error_log_address)

  /* call the callback function and post message */
  event_control_block.event_cb(UNCORRECTABLE, &message);
}

static void sram_error_isr(void) {
  uint8_t minshire = 0;
  uint8_t bank;
  uint64_t error_log_info;
  uint32_t ulMaxID = ioread32(SPIO_PLIC + SPIO_PLIC_MAXID_T0_ADDRESS);

  if ((ulMaxID >= SPIO_PLIC_MINSHIRE_ERR0_INTR) &&
      (ulMaxID <= SPIO_PLIC_MINSHIRE_ERR33_INTR)) {
    minshire = (uint8_t)(ulMaxID - SPIO_PLIC_MINSHIRE_ERR0_INTR);
  } else {
    Log_Write(LOG_LEVEL_CRITICAL, "Wrong interrupt handler");
    return;
  }

  /* Extract error code (single or double bit error) from ERR_LOG_INFO ESR */
  for (bank = 0; bank < 4; bank++) {
     error_log_info = read_esr_new(
         PP_MACHINE, minshire, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
         ETSOC_SHIRE_CACHE_ESR_SC_ERR_LOG_INFO_ADDRESS, bank);
     if (ETSOC_SHIRE_CACHE_ESR_SC_ERR_LOG_INFO_ECC_SINGLE_DOUBLE_VALID_GET(error_log_info)) break;
  }
  uint64_t error_code =
      ETSOC_SHIRE_CACHE_ESR_SC_ERR_LOG_INFO_ECC_SINGLE_DOUBLE_CODE_GET(
          error_log_info);

  if (SINGLE_BIT_ECC == error_code) {
    sram_error_threshold_isr(minshire, bank, error_log_info);
  } else if (DOUBLE_BIT_ECC == error_code) {
    sram_error_uncorr_isr(minshire, bank, error_log_info);
  } else {
    Log_Write(LOG_LEVEL_CRITICAL, "Unexpected error code.");
  }

  /* clear valid bit */
  error_log_info =  ETSOC_SHIRE_CACHE_ESR_SC_ERR_LOG_INFO_ECC_SINGLE_DOUBLE_VALID_MODIFY(error_log_info, 1);
  write_esr_new(PP_MACHINE, minshire, REGION_OTHER,ESR_OTHER_SUBREGION_CACHE,
                ETSOC_SHIRE_CACHE_ESR_SC_ERR_LOG_INFO_ADDRESS, error_log_info, bank);

}

uint16_t Cache_Control_SCP_size(uint64_t shire_mask)
{
    uint64_t scp_cache_ctrl;
    uint16_t bank_scp_size;
    uint8_t  highest_shire_id;
    uint8_t  num_of_active_shires;

    if(0 != shire_mask)
    {
        highest_shire_id = get_highest_set_bit_offset(shire_mask);
    }
    else
    {
        return 0;
    }

    /* All cache banks are configured the same accross the chip, so we are reading
       bank 0 scp cache control register just from one shire */
    scp_cache_ctrl = read_esr_new(PP_MACHINE, highest_shire_id, REGION_OTHER,
                ESR_OTHER_SUBREGION_CACHE, ETSOC_SHIRE_CACHE_ESR_SC_SCP_CACHE_CTL_ADDRESS, 0);

    bank_scp_size = ETSOC_SHIRE_CACHE_ESR_SC_SCP_CACHE_CTL_ESR_SC_SCP_SET_SIZE_GET(scp_cache_ctrl);

    num_of_active_shires = (uint8_t)__builtin_popcountll(shire_mask);

    return (uint16_t)KB_to_MB(num_of_active_shires * SC_BANK_NUM * bank_scp_size);
}

uint16_t Cache_Control_L2_size(uint64_t shire_mask)
{
    uint64_t l2_cache_ctrl;
    uint16_t bank_l2_size;
    uint8_t  highest_shire_id;
    uint8_t  num_of_active_shires;

    if(0 != shire_mask)
    {
        highest_shire_id = get_highest_set_bit_offset(shire_mask);
    }
    else
    {
        return 0;
    }

    /* All cache banks are configured the same accross the chip, so we are reading
       bank 0 l2 cache control register just from one shire */
    l2_cache_ctrl = read_esr_new(PP_MACHINE, highest_shire_id, REGION_OTHER,
                ESR_OTHER_SUBREGION_CACHE, ETSOC_SHIRE_CACHE_ESR_SC_L2_CACHE_CTL_ADDRESS, 0);

    bank_l2_size = ETSOC_SHIRE_CACHE_ESR_SC_L2_CACHE_CTL_ESR_SC_L2_SET_SIZE_GET(l2_cache_ctrl);

    num_of_active_shires = (uint8_t)__builtin_popcountll(shire_mask);

    return (uint16_t)KB_to_MB(num_of_active_shires * SC_BANK_NUM * bank_l2_size);
}

uint16_t Cache_Control_L3_size(uint64_t shire_mask)
{
    uint64_t l3_cache_ctrl;
    uint16_t bank_l3_size;
    uint8_t  highest_shire_id;
    uint8_t  num_of_active_shires;

    if(0 != shire_mask)
    {
        highest_shire_id = get_highest_set_bit_offset(shire_mask);
    }
    else
    {
        return 0;
    }

    /* All cache banks are configured the same accross the chip, so we are reading
       bank 0 l3 cache control register just from one shire */
    l3_cache_ctrl = read_esr_new(PP_MACHINE, highest_shire_id, REGION_OTHER,
                ESR_OTHER_SUBREGION_CACHE, ETSOC_SHIRE_CACHE_ESR_SC_L3_CACHE_CTL_ADDRESS, 0);

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

    if((uint32_t)(scp_size + l2_size + l3_size) > SC_BANK_SIZE)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if(0 != shire_mask)
    {
        num_of_shires = get_highest_set_bit_offset(shire_mask);
    }
    else
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if(((scp_size % 2) != 0) || ((l2_size % 2) != 0) || ((l3_size % 2) != 0))
    {
        return ERROR_INVALID_ARGUMENT;
    }

    if(l2_size < 0x40u)
    {
        return ERROR_INVALID_ARGUMENT;
    }

    /* SCP calculation */
    set_base = 0;
    set_size = scp_size;
    high_bit = get_highest_set_bit_offset((uint64_t)scp_size);
    set_mask = (uint16_t)((0x1u << high_bit) - 0x1u);
    tag_mask = ((0x1u << high_bit) == scp_size ) ? (uint16_t)((0x1u << high_bit) - 0x1u) :
                                                   (uint16_t)((0x1u << (high_bit - 0x1u)) - 0x1u);

    scp_ctl_value =
        ETSOC_SHIRE_CACHE_ESR_SC_SCP_CACHE_CTL_ESR_SC_SCP_SET_BASE_SET(set_base) |
        ETSOC_SHIRE_CACHE_ESR_SC_SCP_CACHE_CTL_ESR_SC_SCP_SET_SIZE_SET(set_size) |
        ETSOC_SHIRE_CACHE_ESR_SC_SCP_CACHE_CTL_ESR_SC_SCP_SET_MASK_SET(set_mask) |
        ETSOC_SHIRE_CACHE_ESR_SC_SCP_CACHE_CTL_ESR_SC_SCP_TAG_MASK_SET(tag_mask);


    /* L2 calculation */
    set_base = scp_size;
    set_size = l2_size;
    high_bit = get_highest_set_bit_offset((uint64_t)l2_size);
    set_mask = (uint16_t)((0x1u << high_bit) - 0x1u);
    tag_mask = ((0x1u << high_bit) == l2_size ) ? (uint16_t)((0x1u << high_bit) - 0x1u) :
                                                  (uint16_t)((0x1u << (high_bit - 0x1u)) - 0x1u);

    l2_ctl_value =
        ETSOC_SHIRE_CACHE_ESR_SC_L2_CACHE_CTL_ESR_SC_L2_SET_BASE_SET(set_base) |
        ETSOC_SHIRE_CACHE_ESR_SC_L2_CACHE_CTL_ESR_SC_L2_SET_SIZE_SET(set_size) |
        ETSOC_SHIRE_CACHE_ESR_SC_L2_CACHE_CTL_ESR_SC_L2_SET_MASK_SET(set_mask) |
        ETSOC_SHIRE_CACHE_ESR_SC_L2_CACHE_CTL_ESR_SC_L2_TAG_MASK_SET(tag_mask);

    /* L3 calculation */
    set_base = (uint16_t)(scp_size + l2_size);
    set_size = l3_size;
    high_bit = get_highest_set_bit_offset((uint64_t)l3_size);
    set_mask = (uint16_t)((0x1u << high_bit) - 0x1u);
    tag_mask = ((0x1u << high_bit) == l3_size ) ? (uint16_t)((0x1u << high_bit) - 0x1u) :
                                                  (uint16_t)((0x1u << (high_bit - 0x1u)) - 0x1u);

    l3_ctl_value =
        ETSOC_SHIRE_CACHE_ESR_SC_L3_CACHE_CTL_ESR_SC_L3_SET_BASE_SET(set_base) |
        ETSOC_SHIRE_CACHE_ESR_SC_L3_CACHE_CTL_ESR_SC_L3_SET_SIZE_SET(set_size) |
        ETSOC_SHIRE_CACHE_ESR_SC_L3_CACHE_CTL_ESR_SC_L3_SET_MASK_SET(set_mask) |
        ETSOC_SHIRE_CACHE_ESR_SC_L3_CACHE_CTL_ESR_SC_L3_TAG_MASK_SET(tag_mask);

    /* Set shire cache configuration */
    for(uint8_t i=0; i<=num_of_shires; i++)
    {
        if (shire_mask & 1)
        {
            /* Set scp cache ctrl */
            write_esr_new(PP_MACHINE, i, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
                ETSOC_SHIRE_CACHE_ESR_SC_SCP_CACHE_CTL_ADDRESS, scp_ctl_value, SC_BANK_BROADCAST);

            /* Set l2 cache ctrl */
            write_esr_new(PP_MACHINE, i, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
                ETSOC_SHIRE_CACHE_ESR_SC_L2_CACHE_CTL_ADDRESS, l2_ctl_value, SC_BANK_BROADCAST);

            /* Set l3 cache ctrl */
            write_esr_new(PP_MACHINE, i, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
                ETSOC_SHIRE_CACHE_ESR_SC_L3_CACHE_CTL_ADDRESS, l3_ctl_value, SC_BANK_BROADCAST);
        }
        shire_mask >>= 1;
    }

    return 0;
}
