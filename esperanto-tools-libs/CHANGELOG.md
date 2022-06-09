# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

[[_TOC_]]

## [Unrealeased]
### Added
- [SW-12975]
  - more tests for memcpy op checks.
### Changed
  - deviceLayer 0.2.0 version hash pinned.
### Deprecated
### Removed
### Fixed
- [SW-12736]
  - fixed memcpy check operation, sometimes it didn't caught some bad memcpy operations (using memory beyond previously reserved).
### Security
## [0.2.0]
### Added
- [SW-12621]
  - added DeviceLayerFake to runtime
- [SW-12315]
  - check device-api compatibility
- this CHANGELOG.md
- [SW-12580]
  - Added CM mask to error handling.
  - StreamError has the new field   `std::optional<uint64_t> cmShireMask_; /// < only available in some kernel errors. Contains offending shiremask`
### Changed
 - *BREAKING CHANGE*: due to check device-api compatibility we need to take care when using IDeviceLayerFake since it doesnt support this command. There is a new option in runtime creation which if set will skip the device-api compatibility check, that needs to be set when using IDeviceLayerFake:

    `static RuntimePtr create dev::IDeviceLayer* deviceLayer, Options options = getDefaultOptions());`

    Instead of relying on default Options, we need to set to false this one (only when using IDeviceLayerFake): `options.checkDeviceApiVersion_=false`

    Example runtime initialization using IDeviceLayerFake:

        dev::IDeviceLayerFake fake;
        auto options = rt::getDefaultOptions();
        options.checkDeviceApiVersion_ = false;
        auto runtime = rt::IRuntime::create(&fake, options);

### Deprecated
### Removed
### Fixed
 - [SW-12696] inconsistency error on CMA memory when running memcpy list DMA operations
### Security


## [0.1.0] -
Initial version; not tracking changes until 0.2.0


