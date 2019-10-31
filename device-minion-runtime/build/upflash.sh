#!/bin/bash

# Uploads all artifacts needed to boot from flash with VaultIP bypassed via OTP

#Uplaod OTP image
scp -C ~/device-firmware/build/zebu_sp_otp*.hex 10.8.32.65:/projects/esperanto/dcrowder/run/otp/

#Upload SP ROM image
scp -C ~/device-firmware/build/src/ServiceProcessorROM/*.hex 10.8.32.65:/projects/esperanto/dcrowder/run/sprom/

#Upload Vault ROM image
scp -C ~/device-firmware/src/ServiceProcessorROM/vaultip_firmware/firmware_eip130_rom_2.5.12.zhex 10.8.32.65:/projects/esperanto/dcrowder/run/vaultrom/

# Upload flash image
scp -C ~/device-firmware/build/flash_16Mbit.hex 10.8.32.65:/projects/esperanto/dcrowder/run/flash/

# Upload kernel
scp -C ~/device-firmware/build/src/kernels/empty/*.hex 10.8.32.65:/projects/esperanto/dcrowder/run/kernel/
