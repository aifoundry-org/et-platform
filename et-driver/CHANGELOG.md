# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

[[_TOC_]]

## [Unreleased]
### Added
### Changed
### Deprecated
### Removed
### Fixed
### Security

## [0.18.0] - 2023-04-27
### Added
### Changed
- Migrated to Ubuntu22
### Deprecated
### Removed
### Fixed
### Security

## [0.17.0] - 2023-12-15
### Added
- [SW-19549] Document the source code and apply checkpatch.pl script
### Changed
### Deprecated
### Removed
### Fixed
### Security

## [0.16.0] - 2023-11-07
### Added
### Changed
- [CI] Adapt to conan pre-release versioning
- conan: remove bundle computation to resolve the conflict
- [SW-18673] Allow reinitiate of device when device appears to be initialized. The use-case is when device state is changed from the backdoor such as PMIC UART, this can help reinitiate the device
### Deprecated
### Removed
### Fixed
- [SW-18407] Fixes for driver instance clean-up on AER error detection
  * Hang-up epoll_wait() by sending EPOLLHUP event to user-space when AER error is detected. The epoll_wait() in user-space will immediately bail-out of wait with event EPOLLHUP
  * Check for device presence in probe() before making any I/O access
- [SW-18720] Fix the enum value of trace execution stack type
- [CI] Run integration-tests on tag creation to upload-driver images
- [CI] Make optional dependencies on development jobs in develop_or_release jobs
### Security

## [0.15.0] - 2023-08-03
### Added
### Changed
- [SW-16846] Update DIRs to match the latest specification
- [CI] Adapt to gitlab.com
### Deprecated
### Removed
### Fixed
- [SW-17528] Fix the alignment of DIRs and use GCC primitives.
### Security

## [0.14.0] - 2023-04-19
### Added
- [SW-16696] Add support for peer-to-peer (P2P) DMA
- [SW-16806] Peer-to-peer (P2P) DMA implementation (loopback driver)
### Changed
- [SW-16696] Consistently use devnum to represent device number and provide devnum through ioctl
### Deprecated
### Removed
### Fixed
### Security

## [0.13.0] - 2023-03-15
### Added
### Changed
### Deprecated
### Removed
### Fixed
- [SW-15696] Fixed recovery for nonfatal AER errors.
### Security

## [0.12.0] - 2023-01-10
### Added
- [SW-5479] Adding support to parse trace buffer full event for mgmt node.
- [SW-15433] Add mechanism for ETSOC reset retries.
- [SW-15589] add sysfs attribute devnum to facilitate the mapping between devnum and PCI slot name.
### Changed
- [SW-15181] `enum dev_state` will not be used internally for maintaining states. The usage is now limited to GET_DEVICE_STATE IOCTL only.
- [SW-15177] Suppressing kernel warning when `dma_alloc_coherent()` cannot allocate requested memory.
- [SW-15589] Changed default permissions to 0666 for `/dev/et*_{mgmt|ops}`.
### Deprecated
### Removed
### Fixed
- [SW-15181] Fixed the following:
  * MM reset workflow when previous MM reset has failed
  * Race condition in restoring device state when reset command submission to SQ has failed
- [SW-15313] Fixed concurrency issue in reset path between devices.
- [SW-15420] Fixed the following bugs when PCI re-initialization fails during ETSOC reset:
  * The device nodes were not getting destroyed on driver unloading after failed PCI re-initialization in ETSOC reset.
  * The 300ms delay was not enough for PCI link to settle. The device was not getting up in this duration. Using pci_device_is_present() API to determine if the device is present. The device can be present immediately after the reset is triggered, then goes down and come up again so using hit-count to make sure that device is present for specific time duration.
### Security

## [0.11.0] - 2022-11-23
### Added
### Changed
- [SW-14614] et_device_api: Changed wait time from u32 to u64
- [SW-9608b] et_device_api: Removed reserved IDs before DMA list commands
- [SW-14868] et_device_api: Removed dependency from device-ops-api message IDs.
- [SW-15075] Reducing device discovery timeouts to 100s
- [SW-15100] et_vma: avoid across device usage of VMA mapping.
- [SW-14867] Saving and restoring the entire PCIe config space for hot reset to remove potential performance degradation.
### Deprecated
### Removed
- [SW-9608] Removed legacy DMA read/write support
### Fixed
### Security

## [0.10.0] - 2022-10-20
### Added
- [SW-14288] Adding support for DM get asic voltage and set module voltage commands in loopback driver.
- [SW-14306] Add ETSOC reset support.
- Updated conanfile to get version from VERSION file.
### Changed
- [SW-14061] Updating the device mgmt api command codes and Get Module Voltage response name.
- (CI) Fetch latest conan docker image
- [SW-14154] Updating DMA response structures to match API structures
- [FV-317] Updating asic_voltage elements to 16bits
### Deprecated
### Removed
### Fixed
- [SW-14453] Fixed crash during driver loading when some msg was already available in the CQ. The spinlock had to be initialized before its usage.
- [SW-10058] Fixed crash during MM/CM trace pull using IO accesses.
### Security

## [0.9.0] - 2022-8-15
### Added
- Enforce updation of CHANGELOG.md and patch version in merge request pipelines
### Changed
- Shift from Jenkins to Gitlab CI
### Deprecated
### Removed
- mem_stats/clear sysfs attribute file
### Fixed
### Security

## [0.8.0] - 2022-6-23
### Added
### Changed
### Deprecated
### Removed
### Fixed
- Passing of LINUX_KMOD_LOCAL_BUILD argument to CI jobs
### Security

## [0.7.1] - 2022-6-23
### Added
- Initial version of changelog file.
### Changed
### Deprecated
### Removed
### Fixed
### Security
