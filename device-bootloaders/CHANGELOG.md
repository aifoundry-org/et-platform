# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

[[_TOC_]]

## [Unreleased]
### Added
- [SW-18887] Added NOC shire re-map functionality
- [SW-20169] Add more logs for fw update
- [SW-226] Added initial support for GET and SET FRU commands
### Changed
- Optimize NOC remap algorithm
- Make FRU data raw binary
### Deprecated
### Removed
### Fixed
### Security

## [0.17.0] - 2024-01-30
### Added
- [SW-19082] Maxion configuration functions.
- Added support to read GPT tables
### Changed
### Deprecated
### Removed
### Fixed
### Security

## [0.16.0] - 2023-12-15
### Added
- [SW-18926] Added multiple fixes to DVFS algorithm for stability
- [SW-19172] Added Linux mode
- EMMC and Maxion configuration only compiled in this mode.
- Changing default log level to WARNING
- Added support to read GPT tables
- Changing default log level to WARNING
### Changed
### Deprecated
### Removed
### Fixed
- Fixed __code_end to be word aligned
### Security

## [0.15.0] - 2023-11-07
### Added
- [SW-18460] Add more error logs for debugging.
- [SW-15352] Check PMIC board type in FOTA process.
- [SW-18602] Added support to rescan a partition after a FW image write.
- [SW-15748] Added a separate DM events category for events directly reported to DM userspace, routed trace buffer full event to DM.
- [SW-18460] Adding voltage verification through PMIC and retries for setting the voltage.
- [SW-18145] Reset the I2C module if it hangs.
  - Added new APIs for enable, disable and abort of I2C interface.
  - Added a workaround to reset the I2C module if it hangs.
- [SW-18674] Send update image slot to PMIC after successful fw update. 
- [SW-19089] Add major versions check in pmic fw update. 
### Changed
- [SW-18208] Enabling PMIC Firmware update.
- [SW-18499] Masking out bad Minion Shire PVT sampling.
- Improving the logging of PMIC FW update.
- Increased BL2 code size to 251K.
- Removing unnecessary inlines and increased the time out for PMIC ready.
- [SW-18592] Update board type checking in PMIC FOTA according to the changed protocol.
### Deprecated
### Removed
- Workaround delays in I2C APIs.
### Fixed
- Fix bug in getting the inactive PMIC slot info
- [SW-18720] Fix bogus value in SP trace header type
- Added the null terminator to the PMIC FW hash.
- [SW-18522] Fixed minor build issues seen with clang
- [SW-19086] Add wait loop after PMIC boot slot update cmd 
- [SW-18926] Fixed handling of thermal idle state
### Security

## [0.14.0] - 2023-09-01
### Added
- Added support for resetting PMIC only if the PMIC firmware update was done.
- [SW-18406] Add locks for changing the voltage and improve error logging.
### Changed
- [SW-18224] Increase the I2C semaphore timeout.
- Remove unnecessary delay in power throttling and adjust a few throttling conditions.
### Deprecated
### Removed
### Fixed
- [SW=16050] Fix intermittent DMA hangs due to NOC marginality
- [SW-18127] Fix initialization of op stats values.
- Fix the I2C communication issue by disabling context switch around the I2C communication APIs.
- [SW-18201] Initializing the frequency values at the startup of system.
- [SW-18234] Fixed throttling bug and added more logging.
- Bugfix: Fixing the throttling idle state condition.
- Reading PMIC FW Hash at boot and fixed bug in reading the correct versions based on slot.
- Fixed pointer to command in MM shell commands.
### Security

## [0.13.0] - 2023-08-03
### Added
- [SW-16663] Adding support to read and transfer PMIC FW image in FW update process.
- [SW-17430]: Adding frequency and voltage stats in op stats
- [SW-16846] Added new 64-bit BAR size fields in DIRs and fixed BAR0 size.
- [SW-17455] Reading PMIC FW version hash before PMIC FW update.
- [SW-17455] Adding support to read and match PMIC FW metadata before update.
    Read and match PMIC FW metadata with current image.
    Do FW update only if the new image does not match.
    Increased the BL2 code size to 444K.
- [SW-17456] Adding PMIC slot 0 and slot 1 image support.
- [SW-17457] Added basic error handling for the firmware update process.
- [SW-17819] Added logs to throttling algorithm
- [SW-17367] First version of eMMC controller.
### Changed
- (CI) Update gitlab-ci-common pointer for Gitlab v15 compatibility
- [SW-17717] Reading the PMIC Firmware image from the flash passive slot.
- Disabling PMIC firmware update temporarily.
- [SW-17377] Update PMIC metadata image structure.
- [CI] Adapt pipeline and conanfile.py to gitlab.com.
- Updating the conan dependencies versions to released components
### Deprecated
### Removed
- [SW-16846] Removed 32-bit BAR size fields in DIRs.
### Fixed
- [SW-17528] Fix the alignment of DIRs and use GCC primitives.
- [SW-17394] Fixed voltage calculation for throttle up and down operations
- [SW_17536] Fix reading minion frequrency from PLLs
- [SW-17538] Update op stats to use frequencies from performance globals
- [SW-17618] Fixed sampling of op_stat to use PVT
- [SW-17565] Fixed sending block of pmic fw update data and increased number of bytes per transaction
- [SW-17727] Fixed issue with Throttle down  status not being updated correcltly
- [SW-17906] Fixed power throtting to properly handle errors retuned form function calls
  - Fixed checking status of operating point change in power throttle
### Security

## [0.12.0] - 2023-06-02
### Added
### Changed
### Deprecated
### Removed
### Fixed
- [SW-17161] Change system boot voltages before bringing up Minions.
- [SW-17139] Change timeouts for PMIC firmware update process.
- [SW-17349] Optimize pmic fw update duration.
### Security

## [0.11.0] - 2023-04-19
### Added
- Adding log error for voltage stability check.
### Changed
- [SW-16695] Change BAR0 size to 32G and map the DDR till end on BAR0.
- Change the verbosity of the SP hearbeat message
- [SW-13951] Enable power throtting based on power requirement
### Deprecated
### Removed
### Fixed
- [SW-16739] Fixes to PMIC FW update code.
  Changed the BL2 code region size.
### Security

## [0.10.0] - 2023-03-15
### Added
- [SW-15389] Adding SP error log on MM FW error before MM reset
- [SW-16234] Adding shire cache control command
- Shire cache configuration command to accept scp, l2 and l3 sizes in MB
- Added more debug information about the state of the PCIe Phy
### Changed
- [SW-15663] Increased pmic timeout and report error on timeout from I2C.
- [SW-15697] Replacing threshold_data with threshold value in trace buffer full notification.
- [SW-16315] Increase boot voltage to 450mV to avoid bit errors
- [SW-16237] Enabled MDI commands in release mode, separated privileged and non privileged memory access regions.
- Fixed a bug in mdi.
- [SW-16453] Generate pre-release versions by default and full versions on tags pipelines
### Deprecated
### Removed
### Fixed
- Fixed a bug in the shire_cache_configuration algo
- Fixed bug in using the right OTP init call
### Security

## [0.9.0] - 2023-01-11
### Added
- [SW-5479] Reporting trace buffer threshold event to host
- [SW-15176] Preserve flash config space with firmware update
- [SW-15176] Separated persistant and non persistant configration structs in BL2 flash config data
- [SW-15559] Adding a stability check loop for pmic set voltage function
### Changed
- Fixing MM reset boot flow.
- [SW-13350] Replacing Trace_String() with Trace_Format_String_V()
- Update the lower limit of esperantoTrace in conan.
- [SW-15614] Report threshold data in trace buffer full event.
- Increased PMIC wait timeout from 3s -> 5s
### Deprecated
### Removed
### Fixed
- [SW-15279] Setting PShire PLLs to 50% frequency to remove intermittent hangs from PCIe DMA engine.
- Remove the validation of config data header from FW update.
### Security

## [0.8.0] - 2022-12-13
### Added
### Changed
- [SW-15175] Removing mem_size from flash config data and reading ddr size and vendor ID correctly.
- Setting concurrent logging of SP alive to critical
- Updating Conan package requirements.
### Deprecated
### Removed
### Fixed
- [SW-15183] Update MemShire PLL programming to support retries similar to other HPDPLL
- Fixing the BP hit wait timeout for fast-boot.
### Security

## [0.7.0] - 2022-11-22
### Added
- [SW-14813] Get and display PMIC FW version at SP boot.
- [SW-14813] Configuring the MPROT ESR for neighboorhoods of all Minion Shires.
- [SW-14272] Add PMIC_READY checks and disabled pmic firmware update test code
- Workaround for handling PMIC_READY in Zebu
### Changed
- Allow FW update even if MM commands are not aborted and change verbosity of some logs.
- (Conan) Consume same signedImageFormat version (1.2.0) as device-minion-rt
- Reformatting the minion_init print log.
### Deprecated
### Removed
### Fixed
- [SW-14630] Reading DDR density and vendor dynamically.
- Updating SRAM voltage to 750 mV
- Fix MDI init sequence.
- Fix DDR and Minion boot changes to address some issues
- Fix RTOS port to avoid any MINION specific switches
### Security

## [0.6.0] - 2022-10-20
### Added
- [SW-13832]: Add support for DM_CMD_GET_MM_STATS
- [SW-13851] add handling for DM_CMD_SET_STATS_RUN_CONTROL
- [SW-14061] Added support for GET_ASIC_VOLTAGES command.
- [SW-14061] Added implementation of get_delta_voltage().
- [SW-6851]: Add support to reset ETSOC through PMIC
- Added more clear debug info for PMIC error
- [SW-14309] Extend Flash template to add PMIC FW img
- [SW-14479] Add API to change module frequency and set safe state frequency to 300
- [SW-14479] Add Macro to set use step clock to set minion frequency
- [SW-14398]: Adding API to send CMD to update minion frequency
- [SW-14534]: Added a pmic write byte API and fixed set pmic reg to use pinter to value for multi byte write
- [SW-14508] Adding get memory size command from MM to SP interface and replace HOST_MANAGED_DRAM_SIZE with ddr_get_memory_size
### Changed
- [SW-14084]: Fixing the CMA calculation to scale up and down the values correctly.
- Support to read PMIC FW from PMIC directly versus hardcoded value
- [SW-14061] Moving the GET_SP_STATS handling to process_performance_request().
- [SW-14061] Separating voltages read from PVT and PMIC.
- [SW-14061] Separated the PMIC and SOC power registers.
- [SW-14061] Reading ASIC voltage for Maxion from PVT.
- [SW-14106] Add checks for the OTP override for PLL setting and also strap options
- [FV-317] Changed asic_voltage struct elements to 16bits
- [SW-14592] PLL0, PLL1 and PSHIRE PLL reprogrammed to 100% target frequency in BL2
- [SW-14480] Minion PVT VM reading of zero removed from Minion AVG calculation
- [SW-14615] Lowering the verbosity of some logs as a WA for trace issue.
- Temporarily disabling the read of vendor ID dynamically due to hang.
- Increase the counter based timeout value for SPI flash accesses.
### Deprecated
### Removed
### Fixed
- Updated PMB stats to handle 4 bytes data read
- Cleaned up PVT inititialization sequence
### Security

## [0.5.0] - 2022-09-02
### Added
- [SW-13953]: Adding eviction of SP stats and trace buffer to avoid trace header update delay
- [SW-14044]: Addition of op stats init function and refactoring update temperature stats function
### Changed
- [SW-13850] Disabled power throttling code in update_module_soc_power() until SW-13951 is resolved.
- Updating NOC, DDR and Maxion boot voltages
- [SW-13954] Add handling for DM_CMD_SET_MODULE_PART_NUMBER
- [SW-13954] Add handling for DM_CMD_SET_MODULE_VOLTAGE
- [SW-13954] Using PMIC voltage values for Maxion, PCIe Logic, VDDQ and VDDQLP.
- [SW-14083] Update power status structure to use current power as 16 bits instead of 8
- Lowered the verbosity of a log.
- [SW-14115] Changing the power value reading routines to read raw power values from PMIC in 10 mW steps.
- Changing Maxion boot voltage to 600mV.
- PMIC FW version updated to 0.6.2.
### Deprecated
### Removed
- Removed trace buffer evict from reset.
### Fixed
- [SW-13571] Fixed watchdog timeout calculations with respect to pclk
- [SW-13974] Updating the PCIe driver to poll on bar addresses until assigned with delays.
- [SW-13978]: Refactored set frequency function
- Adding locks in SP trace buffer evict and reduced versbosity of some logs.
### Security

## [0.4.0] - 2022-08-16
### Added
- Added locks while configure/reset SP trace
- [SW-13253] Added support to read Minion, NOC and SRAM current from PMIC
- [SW-13035] Enforce updation of CHANGELOG.md and patch version in merge request pipelines
- [SW-13284] Adding locks to I2C driver and minor fixes in soc power
- [SW-13253] Adding functionality to read PMB stats from PMIC
- [SW-13253] Workaround for flash init for fast boot.
- [SW-13466] Porting over changes from silicon branch.
- [SW-6113] Add doxygen support for sp_host_iface.h and mm_iface.h.
- Extend support to read instantenous power, and added delay to PMIC reg access
- Updated DDR frequency from 933 Mhz to 1066 Mhz
- [SW-13620] Add implementation for DM_CMD_GET_SP_STATS
- [SW-13788] Forcing all shires to be initialized for silicon. Workaround for VPURF bug.
- [SW-13788] Clearing RF F0-F31 in VPU RF init.
### Changed
- [SW-11649] Separate out Trace and Serial log string max size.
- Conanfile now declares dependencies with version-ranges
- Changed DDR Frequency to 933MHz.
### Deprecated
### Removed
- [SW-12973] Auto reset of MM from MM heartbeat watchdog expiry.
- Removed Thermal_Pwr_Mgmt_Update_Sample_counter() since its no longer required.
### Fixed
- [SW-11649] Fixed usage of Trace string length to be not fixed size.
- [SW-13218] Reading memory size from flash in get_memory_size().
  * Reading dummy vendor ID until reading from chip register is fixed.
- [SW-13220] Updates to ensure the correct value is returned for PCIe max link speed. It was returning value for one older GEN.
- Override MNN, SRM, NOC voltage from SP to enable optimized operation point
- Throttle power state test to update the trace header.
- [SW-13253] updated get module voltage to use values from pvt
- [FV-296, SW-13251, SW-13273] SW WA for FV-296
- [SW-13273] Fix for PLL frequency calculation
- [SW-13253] Fixed delays for sysemu by introducing delay macro
- [FV-296] Fix of SW WA for FV-296
- [SW-13479] Fix for Zebu FAST_BOOT
- [SW-12973] Initialized the MM heartbeat timer after the PCIe initialization is complete.
- [SW-12853] Updated sequence changes in MM reset flow
- [SW-13583] Fixed updating completed boot counters
### Security

## [0.3.0] - 2022-6-25
### Added
### Changed
### Deprecated
### Removed
### Fixed
### Security

## [0.2.3] - 2022-6-24
### Added
- Initial version of changelog file.
- Logs for TDP get.
### Changed
### Deprecated
### Removed
### Fixed
### Security
