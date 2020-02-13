#ifndef __BL2_SP_OTP_H__
#define __BL2_SP_OTP_H__

#include <stdint.h>
#include <stdbool.h>

#include "sp_otp_data_layout.h"

int sp_otp_init(void);
int sp_otp_read(uint32_t offset, uint32_t * result);
int sp_otp_write(uint32_t offset, uint32_t value);

int sp_otp_get_pll_configuration_data(OTP_PLL_CONFIGURATION_OVERRIDE_t * table, uint32_t table_size, uint32_t * count);
int sp_otp_get_uart_configuration_data(OTP_UART_CONFIGURATION_OVERRIDE_t * configuration);
int sp_otp_get_spi_configuration_data(OTP_SPI_CONFIGURATION_OVERRIDE_t * pll_100,
                               OTP_SPI_CONFIGURATION_OVERRIDE_t * pll_75,
                               OTP_SPI_CONFIGURATION_OVERRIDE_t * pll_50,
                               OTP_SPI_CONFIGURATION_OVERRIDE_t * pll_off);
int sp_otp_get_flash_configuration_data(OTP_FLASH_CONFIGURATION_OVERRIDE_t * spi0, OTP_FLASH_CONFIGURATION_OVERRIDE_t * spi1);

// return list of entries that should be appended to the PCIe whitelist
int sp_otp_get_pcie_whitelist_configuration_data(OTP_PCIE_WHITELIST_ENTRY_OVERRIDE_t * table, uint32_t table_size, uint32_t * count);

// return a list of entires that should be applied before the PCIe configuration
int sp_otp_get_pre_configuration_data(OTP_PCIE_WHITELIST_ENTRY_OVERRIDE_t * table, uint32_t table_size, uint32_t * count);

// return a list of entires that should be applied after the PCIe configuration
int sp_otp_get_post_configuration_data(OTP_PCIE_WHITELIST_ENTRY_OVERRIDE_t * table, uint32_t table_size, uint32_t * count);

int sp_otp_get_sp_issuing_ca_certificate_monotonic_version_counter(uint32_t * counter);
int sp_otp_get_pcie_cfg_data_certificate_monotonic_version_counter(uint32_t * counter);
int sp_otp_get_sp_bl1_certificate_monotonic_version_counter(uint32_t * counter);

int sp_otp_get_vaultip_chicken_bit(bool * disable_vault);
int sp_otp_get_vaultip_plain_text_firmware_chicken_bit(bool * allow_plain_text_firmware);
int sp_otp_get_vaultip_clock_switch_chicken_bit(bool * switch_clocks, uint32_t * clock_switch_input_token);
int sp_otp_get_signatures_check_chicken_bit(bool * ignore_signatures);
int sp_otp_get_vaultip_FIPS_mode(bool * use_FIPS_mode);
int sp_otp_get_engineering_mode(bool * allow_engineering_keys);

int sp_otp_get_pcie_cfg_white_list_check_chicken_bit(bool * ignore_white_list);
int sp_otp_get_vaultip_firmware_check_start_timeout(bool * use_otp_timeout, uint32_t * timeout);
int sp_otp_get_vaultip_firmware_accepted_timeout(bool * use_otp_timeout, uint32_t * timeout);
int sp_otp_get_vaultip_firmware_output_token_timeout_1(bool * use_otp_timeout, uint32_t * timeout);
int sp_otp_get_vaultip_firmware_output_token_timeout_2(bool * use_otp_timeout, uint32_t * timeout);
int sp_otp_get_vaultip_firmware_output_token_timeout_3(bool * use_otp_timeout, uint32_t * timeout);
int sp_otp_get_vaultip_firmware_output_token_timeout_4(bool * use_otp_timeout, uint32_t * timeout);
int sp_otp_get_vaultip_firmware_output_token_timeout_5(bool * use_otp_timeout, uint32_t * timeout);

int sp_otp_get_vaultip_clock_switch_token(bool * use_clock_switch, uint32_t * token);

int sp_otp_get_sp_l1_cache_chicken_bit(bool * enable_l1_cache);

int sp_otp_get_special_customer_designator(uint8_t * designator);
int sp_otp_get_critical_patch_data(uint32_t index, OTP_CRITICAL_PATCH_t * patch_data);

void sp_otp_diag(void);

#endif
