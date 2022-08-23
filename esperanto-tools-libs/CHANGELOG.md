# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

[[_TOC_]]
## [Unrealeased]
### Added
### Changed
### Deprecated
### Removed
### Fixed
### Security
## [0.5.0]
### Added
### Changed
### Deprecated
- [SW-13472] removed deprecated handling and usage of device-api old structs.
### Removed
### Fixed
- added missing dependency libcap.
- [SW-13861] fixed releasing execution cache buffer when there were no abortkernel callback registered.
### Security
## [0.4.0]
### Added
- [SW-12942] implemented a check in host for kernel shire mask
- Added new error code for kernel launches with invalid shire mask
- [SW-11138] added some exceptions catch inside runtime threads which could cause the server to hang.
- [SW-13139] Added getDmaInfo API; which returns a new types. See "runtime/Types.h". The new API call is:  
  ```cpp
    virtual DmaInfo getDmaInfo(DeviceId deviceId) const = 0;  
  ```
- small performance improvement to profiler
- [SW-13614] refactored runtime internal implementation to follow NVI idiom; easing the implementation of common parts -profiling- (kind of template pattern).
- [SW-13533] added profiling in client side. This profiling does not include internals, only runtime API.
- [SW-11402] and [SW-10777] added a new factory method to instantiate runtime client (instead of runtime standalone)
  ```cpp
  ///
  /// \brief Factory method to instantiate a client IRuntime implementation
  ///
  /// @param[in] socketPath indicates which socket the Client will connect to
  ///
  /// @returns RuntimePtr an IRuntime instance. See \ref dev::IDeviceLayer
  ///
  static RuntimePtr create(const std::string& socketPath);
  ```
  - all tickets from [SW-10470]

### Changed
- Runtime will check device-api version only for major and a minor minimum. Its device-api implementors responsability to respect the semversion and make it compatible for any version with the same major.
- [SW-13523] updated DeviceProperties serialization and struct (see "runtime/Types.h") to expose two new fields:
  * tdp
  * form factor
- deviceLayer 0.3.0 version hash pinned.
- Convert from KB to MB the device sizes for L2 and L3 
- Changed gitlab configuration to allow more memory in kubernetes instances
- IRuntime interface has changed but these are no breaking changes:
  * added CmaCopyFunction as an optional parameter to customize the way the data is copied from user-space virtual memory to CMA buffers. This is intended to be used internally and defaults to regular std::copy function.
### Deprecated
### Removed
- ***BREAKING CHANGE***: device fw tracing
  - runtime won't pull firmware traces automatically on initialization
  - all device firmware trace related methods will be removed: https://esperantotech.atlassian.net/browse/SW-10843 and https://esperantotech.atlassian.net/browse/SW-11156
  methods affected are:
  ```cpp
  EventId setupDeviceTracing(StreamId stream, uint32_t shireMask, uint32_t threadMask, uint32_t eventMask, uint32_t filterMask, bool barrier = true);
  EventId startDeviceTracing(StreamId stream, std::ostream* mmOutput, std::ostream* cmOutput, bool barrier = true);
  EventId stopDeviceTracing(StreamId stream, bool barrier = true);

  ```
- ***BREAKING CHANGE***: tracing:
  - profileEvent Class::LoadCode "loadAddress" will indeed contain the loadAddress instead of the entryPoint. This is actually a FIX but changes the old behavior
  - profileEvent Class::KernelLaunch won't trace a meaningful loadAddress anymore. Still the field will be fill but with a -1 (0xFFFFFFFFFFFFFFFF)

- ***BREAKING CHANGE***: [SW-13511] ***removed*** IDmaBuffers from the runtime API. All three following methods have been removed from the API:
  ```cpp
   virtual EventId memcpyHostToDevice(StreamId stream, const IDmaBuffer* h_src, std::byte* d_dst, size_t size, bool barrier = false)
   virtual EventId memcpyDeviceToHost(StreamId stream, const std::byte* d_src, IDmaBuffer* h_dst, size_t size,bool barrier = true) = 0;
   virtual std::unique_ptr<IDmaBuffer> allocateDmaBuffer(DeviceId device, size_t size, bool writeable) = 0;
  ```

### Fixed
- Adapt device tests to return L2 and L3 sizes in KB
### Security
## [0.3.2]
### Added
### Changed
  - deviceLayer 0.3.0 version hash pinned.
  - Added new error code for kernel launches with invalid shire mask
  - Convert from KB to MB the device sizes for L2 and L3 
### Deprecated
### Removed
### Fixed
  - Adapt device tests to return L2 and L3 sizes in KB
### Security
## [0.3.1]
### Added
- [SW-13062] waitForEvent / waitForStream won't block if timeout argument is 0 seconds
### Changed
### Deprecated
### Removed
### Fixed
### Security
## [0.3.0]
### Added
- [SW-13045] added a new callback which will be executed when a kernel abort happens. See new API function: 
```cpp
virtual void setOnKernelAbortedErrorCallback(const KernelAbortedCallback& callback) = 0;
```
### Changed
### Deprecated
### Removed
### Fixed
### Security
## [0.2.4]
### Added
### Changed
  - removed device-api check because system-sw needs to use an untested device-api for client release.
### Deprecated
  - release [0.2.3] was a "hotfix" to enable system-sw to do a merge without checking device-api. Problem is that it was not built on top of 0.2.2 but on top of unstable/untested master. So its encouraged to not use that release.
### Removed
### Fixed
### Security
## [0.2.2]
### Added
- deviceLayer 0.3.0 version hash pinned.
- Added new error code for kernel launches with invalid shire mask
- Convert from KB to MB the device sizes for L2 and L3 
- Adapt device tests to return L2 and L3 sizes in KB

### Changed
### Deprecated
### Removed
### Fixed

## [0.2.1]
### Added
- [SW-12975] 
  - more tests for memcpy op checks. 
### Changed
  - deviceLayer 0.2.0 version hash pinned.
### Deprecated
### Removed
### Fixed
- [SW-12900]
  - try to merge memcpylists to avoid deadlocks waiting for huge memcpylists
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
  - StreamError has the new field   
  - ```cpp 
    std::optional<uint64_t> cmShireMask_; /// < only available in some kernel errors. Contains offending shiremask
    ``` 
### Changed
 - *BREAKING CHANGE*: due to check device-api compatibility we need to take care when using IDeviceLayerFake since it doesnt support this command. There is a new option in runtime creation which if set will skip the device-api compatibility check, that needs to be set when using IDeviceLayerFake:

    ```cpp 
    static RuntimePtr create dev::IDeviceLayer* deviceLayer, Options options = getDefaultOptions());
    ```

    Instead of relying on default Options, we need to set to false this one (only when using IDeviceLayerFake): `options.checkDeviceApiVersion_=false`

    Example runtime initialization using IDeviceLayerFake:
    ```cpp
    dev::IDeviceLayerFake fake;
    auto options = rt::getDefaultOptions();
    options.checkDeviceApiVersion_ = false;
    auto runtime = rt::IRuntime::create(&fake, options);
    ```
### Deprecated
### Removed
### Fixed
 - [SW-12696] inconsistency error on CMA memory when running memcpy list DMA operations
### Security


## [0.1.0] -
Initial version; not tracking changes until 0.2.0


