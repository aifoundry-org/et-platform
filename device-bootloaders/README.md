Service Processor Bootloaders (BL1 and BL2) Project
=======================

This project implements the Service Processor bootloaders running on the Esperanto SOC's Service Processor.

## WARNING

Currently, this repo is not self-contained and it should be built as part of the sw-platform's build flow.

## BL2 variants

# Production variant

Code that will go into production and run on the silicon.

# Fast-boot variant

Some initialization steps have been disabled/by-passed to make it more suitable for developing and running on hardware/software simulators.

The changes in comparison to the production variant are:

* Contains a "fake BL1-DATA" (`SERVICE_PROCESSOR_BL1_DATA_t`)
* Expects the SPIO UART0 to not be initialized, therefore it initializes it
* Skips FlashFS initialization (which requires SPI)
* Expects the PCIe link to be down, therefore it performs full PCIe link initialization
* Skips DDR controller configuration
* Skips loading Minion runtime firmware images from the flash, it assumes they are already pre-loaded to DDR
