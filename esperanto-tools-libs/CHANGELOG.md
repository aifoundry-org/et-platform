# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

[[_TOC_]]
## [unreleased]
### Added
### Changed
### Deprecated
### Removed
### Fixed
- [SW-19729] Fix client hangs when messages arrive out of order
- [SW-19262] Fix some runtime tests timeout when the host is very loaded

## [0.13.1]
### Fixed
- [SW-19289] Fail early if stack size is too small

## [0.13.0]
### Added
- [SW-19325] Use new kernelLaunchOptionsImp struct on call client/server
- [SW-19289] Implement mechanism for a configurable stack
- [SW-19169] Implement more flexible API for launching kernel
### Fixed
- [SW-18922] Fix missing thread 0 in coredumps
- [SW-19540] Fix handling of reordered responses in client mode
- [SW-19575] Fix hang during initialization with binary mode sysemu2
- [SW-19586] Fix the BSS test

## [0.12.3]
### Fixed
- [SW-19346] Fixed aborting kernels.

## [0.12.2]
### Fixed
- [SW-18582] Fixed core dumps generation when kernel aborts.

## [0.12.1]
### Fixed
- [SW-18212] Fix missing hostUtils::actionList in package_info components

## [0.12.0]
### Added
- [SW-16043] add coredump feature. Bumped protocol to 3.2.
### Changed
- [SW-18125] bumped minimum device-api requirement to 2.0.0

## [0.11.0]
### Added
- [SW-17092] added compatibility with devicelayer 2.1 and device api 1.4
- [SW-17093][SW-17094] add new runtime interface to support DeviceToDevice memory transfers
- [SW-17095] create deviceToDevice integration tests
- [SW-17079] now any waitForStream will block until callbacks associated to that stream (if any) have been fully executed.
- [SW-17351] Added tracing for D2D operations.
## Fixed
- [SW-17344] fixed kernelAbort callback not being called when aborting a kernel in MP flow

## [0.10.0]
### Added
- [SW-15675] Added easyprofiler support.
- [SW-16963] new requests for the server-client protocol; also improves the protocol versioning system.
- [SW-17011] new IMonitor interface.
- [SW-17066] unit tests for the new IMonitor interface.
## Fixed
 - [SW-16519] Fixed an issue freeing events in the multiprocess flow which made each subsequent waitForStream (for any stream) take longer and longer.

## [0.9.2]
## Fixed
 - Removed StreamError breaking change; it gets a -1 as a device default parameter.
 - [SW-16687] Fixed bug in MemcpyListH2D.

## [0.9.1]
## Fixed
 - [SW-16619] There is a responsereceiver thread per device. This thread is polling for responses (since interrupts or don't work or don't are performant enough.). The thread was quite agressively polling, tunning the polling times has fixed the leap in CPU usage.
At the same time I have tuned down the CMA pool, since we have seen a huge increase in required time to allocate bigger chunks of CMA.

## [0.9.0]
### Added
- [SW-15922] new sync mechanism is cleaner and less error prone than the older one. Threadpools are using just for task execution, as they should be, and all sync points are done using events and not blocking workers from the threadpool. This also brings some nice performance improvements, benchmarks for memory transactions H2D and D2H gives a 1.6x - 1.8x speedup.
- [SW-15882] adapted to changes in the devicelayer api (2.0.0)
- [SW-16044] added deviceId and streamId to StreamError. Note streamId will not be present when recovering from a crash.
- [SW-16249] DeviceLayerFake is now parametrizable. MP server new parameters: num-devices (valid for sysemu and fake), frequency (valid for fake), dramsize (valid for fake).
### Changed
- [SW-16264] Adapt CI to allow package creation through tags

## [0.8.0]
### Added
- [SW-15494] Added an individual threadpool for each device
- [SW-15364] Automatic doxygen documentation generation.
- [SW-15254] CMA allocation policy updated.
- [SW-15419] Avoid using CMA for loading kernels when not needed.
- [SW-15613] Removed all general locks from memcpyops. Improved cmaManager(s).
- [SW-15649] Added doxyfile.in and some changes to documentation generation.
- [SW-15662] ResponseReceiver now has 1 thread for each device (before it was a single thread for all devices).
- [SW-15961] Changed 16 GiB -> 32 GiB on deviceLayerFake
- [SW-15925] Added two new trace events: CmaCopy and CmaWait. Bumped trace version to 2.
### Removed
- [SW-15649] Removed generated files
### Fixed
- Some documentation cross references
- [SW-15881] Fixes data race when checking memory operation. This bug was not present in previous released versions, so no hot fixes are needed.
- [SW-15901] Introduces locks back again to ensure no lower prio commands block inside the threadpools without leaving room for high priority commands to execute. This bug is not present in previous runtime releases AFAIK.
## [0.7.1]
### Added
- [SW-15419] Avoid using CMA for loading kernels when not needed.
## [0.7.0]
### Added
- [SW-15123] Added Start and Stop events to runtime traces. Added parentId field for commands derived from a single application-level eventId.
- [SW-14909] Added API method to set profiler in Server (runtime mp daemon).
- [SW-14909] Added couple of python scripts to DMA performance evaluation.
- [SW-15329] DeviceLayerFake now supports multiple devices
### Changed
- deviceApi minimum required 1.0.0. Due to some changes in firmware device-api 1.0.0 is not backwards compatible.
- (Conan) Refactor sysemu artifacts provided when runtime:with_tests=True
- (Conan) Update accepted range of deviceLayer versions from `>=1.1.0 <1.2.0` to `>=1.1.0 <2.0.0`
### Removed
- **breaking change:** Removed getPairId because it was not used. Inside runtime itself ...
### Fixed
- (Conan) Fix package_info requires
- [SW-15322] Unlock earlier in mem operations involving memcpylist to improve the latency

## [0.6.5]
### Changed
- Reverted device-api 1.0.0 requirement back to 0.>=6.X
### Fixed
- [SW-15322] Bring latency improvements when doing memcpy operations with memcpylists. These improvements were originally released in 0.7

## [0.6.4]
### Changed
- deviceApi minimum required 1.0.0. Due to some changes in firmware device-api 1.0.0 is not backwards compatible.
- Update Conan dependencies.
## [0.6.3]
### Fixed
- [SW-15083] change from a single CMA memory pool to one per device

## [0.6.2]
### Changed
- [SW-14803] implemented a new way for loading kernels. This has caused a change version in the protocol.
**breaking change:**  new protocol version 2. There is not backwards compatibility.
## [0.6.1]
### Fixed
- [SW-14771] this fixes the hang for ioThread.

## [0.6.0]
### Added
- [SW-14117] added support for benchmark tool using new runtime server
- [SW-14093] added new docker image which will host the server multiprocess instance.
- [SW-14053] added new kernel error handling from device api 0.6.0. Bumped system-sw projects to work with device api 0.6.0.
- [SW-14415] added a IO thread to process the serialization and writing of the traces in background. Improvement (when traces enabled and JSON format) of ~35% in some heavy runtime intensive workloads (ie. DLRM_RMC1)
### Changed
- (Conan) (CI) Updated build flow to conan 1.52.0
- (Conan) Depend on sw-sysemu/0.2.1
- [SW-14434] (Conan) CI now generates Debug & Release pkgs
- *** Breaking Change *** changed ArchRevision from DeviceProperties to be consistent with DeviceLayer
- Adapted to deviceLayer breaking changes
### Fixed
- [SW-14527] fixed race condition on dispatch event where the profiler could be gone if the runtime is already destroyed.
- [SW-13976] NOSYSEMU tests tunned down, now running al PCIE MP tests (including "NOSYSEMU" tests) takes less than 6mins.

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

[Unreleased]: https://gitlab.esperanto.ai/software/esperanto-tools-libs/-/compare/0.6.1...master

[0.6.1]: https://gitlab.esperanto.ai/software/esperanto-tools-libs/-/compare/0.6.0...0.6.1
[0.6.0]: https://gitlab.esperanto.ai/software/esperanto-tools-libs/-/compare/0.5.0...0.6.0
[0.5.1]: https://gitlab.esperanto.ai/software/esperanto-tools-libs/-/compare/0.5.0...0.5.1
[0.5.0]: https://gitlab.esperanto.ai/software/esperanto-tools-libs/-/compare/0.4.0...0.5.0
[0.4.0]: https://gitlab.esperanto.ai/software/esperanto-tools-libs/-/compare/0.3.2...0.4.0
[0.3.2]: https://gitlab.esperanto.ai/software/esperanto-tools-libs/-/compare/0.3.1...0.3.2
[0.3.1]: https://gitlab.esperanto.ai/software/esperanto-tools-libs/-/compare/0.3.0...0.3.1
[0.3.0]: https://gitlab.esperanto.ai/software/esperanto-tools-libs/-/compare/0.2.4...0.3.0
[0.2.4]: https://gitlab.esperanto.ai/software/esperanto-tools-libs/-/compare/0.2.3...0.2.4
[0.2.3]: https://gitlab.esperanto.ai/software/esperanto-tools-libs/-/compare/0.2.2...0.2.3
[0.2.2]: https://gitlab.esperanto.ai/software/esperanto-tools-libs/-/compare/0.2.1...0.2.2
[0.2.1]: https://gitlab.esperanto.ai/software/esperanto-tools-libs/-/compare/0.2.0...0.2.1
[0.2.0]: https://gitlab.esperanto.ai/software/esperanto-tools-libs/-/compare/0.1.0...0.2.0
[0.1.0]: https://gitlab.esperanto.ai/software/esperanto-tools-libs/-/tags/0.1.0

