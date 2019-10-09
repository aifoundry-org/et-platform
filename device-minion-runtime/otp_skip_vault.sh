#!/bin/bash

# Create a blank OTP file
src/ServiceProcessorROM/scripts/sp_otp_editor.py create-blank-otp-file build/otp.bin

# Set the "VaultIP disable" chicken bit
src/ServiceProcessorROM/scripts/sp_otp_editor.py write-set build/otp.bin src/ServiceProcessorROM/scripts/sp_otp_template.json "Chicken Bits" "VIP DIS" 1

# Convert to Zebu format
# Will emit 3 files, build/zebu_sp_otp[345].hex
src/sp_otp_convert.py B2Z build/otp.bin build/zebu_sp_otp
