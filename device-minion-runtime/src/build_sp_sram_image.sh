#!/bin/bash

set -e 

RAM_IMAGE_PATH=~/sp_ram_image.bin
HEX_FILE_PREFIX=sp_sram
BIN_FILE_PREFIX=sp_sram
HEX_IMAGE_PATH=~/${HEX_FILE_PREFIX}
BIN_IMAGE_PART=~/${BIN_FILE_PREFIX}

PCIE_CFG_PATH=~/source/device-firmware/firmware-tools/test_files/test_pcie_config_data.img
#PCIe HDR @0x40406550,2816
#PCIe DATA @0x40408d30,128
PCIE_CFG_HEADER_OFFSET=$((0x06550))
PCIE_CFG_HEADER_SIZE=$((2816))
PCIE_CFG_DATA_OFFSET=$((0x08d30))

#VIP FW @0x40412f28,82240
VIP_FW_PATH=~/source/device-firmware/src/ServiceProcessorROM/vaultip_firmware/firmware_eip130_ram_2.5.12_rom_keys.sbi
VIP_FW_OFFSET=$((0x12f28))

#CRT @0x40407064,3424
CRT_PATH=~/source/device-firmware/firmware-tools/code-signing-tools/test_templates_and_certificates/test_sp.crt
CRT_OFFSET=$((0x07064))
CRT_SIZE=$((3424))

BL1_PATH=~/source/device-firmware/firmware-tools/code-signing-tools/bl1.img
#BL1 HDR @0x40407dc4,3072
#BL1 DATA @0x404e0000,9216
BL1_HEADER_OFFSET=$((0x07dc4))
BL1_HEADER_SIZE=$((3072))
BL1_DATA1_OFFSET=$((0xe0000))
BL1_DATA1_SIZE=$((9216))
#BL1_DATA2_OFFSET=$((0xf0000))
#BL1_DATA2_SIZE=$((256))
#BL2_DATA2_SKIP=$((BL1_HEADER_SIZE + BL1_DATA1_SIZE))

CHICKEN_BIT=~/skip_flash_chicken_bit.bin
CHICKEN_BIT_OFFSET=$((0x3FFF0))

MAKE_SP_SRAM=~/source/device-firmware/src/make_sp_sram.py

##########################################################################################################################################
## Initialize
##########################################################################################################################################

echo "Zero-initialize the SP RAM image ${RAM_IMAGE_PATH}"
dd bs=1024 if=/dev/zero of=~/sp_ram_image.bin count=1024
echo ""

##########################################################################################################################################
## PCIe Config Data
##########################################################################################################################################

echo "Write the PCIe header from ${PCIE_CFG_PATH} at offset ${PCIE_CFG_HEADER_OFFSET}, size ${PCIE_CFG_HEADER_SIZE}"
dd if=${PCIE_CFG_PATH} of=${RAM_IMAGE_PATH} bs=1 conv=notrunc seek=${PCIE_CFG_HEADER_OFFSET} count=${PCIE_CFG_HEADER_SIZE}
echo ""

echo "Write the PCIe data from ${PCIE_CFG_PATH} at offset ${PCIE_CFG_DATA_OFFSET} (input skip ${PCIE_CFG_HEADER_SIZE})"
dd if=${PCIE_CFG_PATH} of=${RAM_IMAGE_PATH} bs=1 conv=notrunc seek=${PCIE_CFG_DATA_OFFSET} skip=${PCIE_CFG_HEADER_SIZE}
echo ""

##########################################################################################################################################
## VaultIP FW
##########################################################################################################################################

echo "Write the VIP FW from ${VIP_FW_PATH} at offset ${VIP_FW_OFFSET}"
dd if=${VIP_FW_PATH} of=${RAM_IMAGE_PATH} bs=1 conv=notrunc seek=${VIP_FW_OFFSET}
echo ""

##########################################################################################################################################
## CRT Chain
##########################################################################################################################################

echo "Write the CRT Chain from ${CRT_PATH} at offset ${CRT_OFFSET}"
dd if=${CRT_PATH} of=${RAM_IMAGE_PATH} bs=1 conv=notrunc seek=${CRT_OFFSET}
echo ""

##########################################################################################################################################
## BL1 Image
##########################################################################################################################################

echo "Write the BL1 header from ${BL1_PATH} at offset ${BL1_HEADER_OFFSET}, size ${BL1_HEADER_SIZE}"
dd if=${BL1_PATH} of=${RAM_IMAGE_PATH} bs=1 conv=notrunc seek=${BL1_HEADER_OFFSET} count=${BL1_HEADER_SIZE}
echo ""

echo "Write the BL1 data 1 from ${BL1_PATH} at offset ${BL1_DATA1_OFFSET}, size ${BL1_DATA1_SIZE} (input skip ${BL1_HEADER_SIZE})"
dd if=${BL1_PATH} of=${RAM_IMAGE_PATH} bs=1 conv=notrunc seek=${BL1_DATA1_OFFSET} skip=${BL1_HEADER_SIZE}
echo ""

#echo "Write the BL1 data 2 from ${BL1_PATH} at offset ${BL1_DATA2_OFFSET}, size ${BL1_DATA2_SIZE} (input skip ${BL2_DATA2_SKIP})"
#dd if=${BL1_PATH} of=${RAM_IMAGE_PATH} bs=1 conv=notrunc seek=${BL1_DATA2_OFFSET} skip=${BL2_DATA2_SKIP}
#echo ""

##########################################################################################################################################
## BL1 Image
##########################################################################################################################################

echo "Write the CHICKEN_BIT from ${CHICKEN_BIT} at offset ${CHICKEN_BIT_OFFSET}"
dd if=${CHICKEN_BIT} of=${RAM_IMAGE_PATH} bs=1 conv=notrunc seek=${CHICKEN_BIT_OFFSET}
echo ""

##########################################################################################################################################
## Finalize
##########################################################################################################################################

echo "Split SP RAM image into 4 parts"
dd if=${RAM_IMAGE_PATH} of=${BIN_IMAGE_PART}_0.bin bs=1024 count=256 skip=0
dd if=${RAM_IMAGE_PATH} of=${BIN_IMAGE_PART}_1.bin bs=1024 count=256 skip=256
dd if=${RAM_IMAGE_PATH} of=${BIN_IMAGE_PART}_2.bin bs=1024 count=256 skip=512
dd if=${RAM_IMAGE_PATH} of=${BIN_IMAGE_PART}_3.bin bs=1024 count=256 skip=768
echo ""

echo "Convert the SP RAM image to ZeBu HEX format"
${MAKE_SP_SRAM} ${HEX_IMAGE_PATH} ${BIN_IMAGE_PART}
echo ""

echo "Copy ZeBu HEX SP RAM images"
scp -C ${HEX_IMAGE_PATH}_*.hex 10.8.32.65:/projects/esperanto/patryk/et-project/run
