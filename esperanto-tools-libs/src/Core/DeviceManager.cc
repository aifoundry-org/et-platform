//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "Core/DeviceManager.h"
#include "Core/Device.h"
#include "PCIEDevice/PCIeDevice.h"
#include "Support/Logging.h"

#include "etrt-bin.h"

#include <cstring>
#include <memory>

using namespace std;

/*
 * @brief Initalization values for an empty Device properties struct
 */
static const struct etrtDeviceProp etrtDevicePropDontCare = {
    {'\0'},    /* char   name[256];               */
    0,         /* size_t totalGlobalMem;          */
    0,         /* size_t sharedMemPerBlock;       */
    0,         /* int    regsPerBlock;            */
    0,         /* int    warpSize;                */
    0,         /* size_t memPitch;                */
    0,         /* int    maxThreadsPerBlock;      */
    {0, 0, 0}, /* int    maxThreadsDim[3];        */
    {0, 0, 0}, /* int    maxGridSize[3];          */
    0,         /* int    clockRate;               */
    0,         /* size_t totalConstMem;           */
    -1,        /* int    major;                   */
    -1,        /* int    minor;                   */
    0,         /* size_t textureAlignment;        */
    0,         /* size_t texturePitchAlignment    */
    //-1,        /* int    deviceOverlap;           */
    0,      /* int    multiProcessorCount;     */
    0,      /* int    kernelExecTimeoutEnabled */
    0,      /* int    integrated               */
    0,      /* int    canMapHostMemory         */
    0,      /* int    computeMode              */
    0,      /* int    maxTexture1D             */
    0,      /* int    maxTexture1DMipmap       */
    0,      /* int    maxTexture1DLinear       */
    {0, 0}, /* int    maxTexture2D[2]          */
    {
        0,
        0,
    },         /* int    maxTexture2DMipmap[2]    */
    {0, 0, 0}, /* int    maxTexture2DLinear[3]    */
    {0, 0},    /* int    maxTexture2DGather[2]    */
    {0, 0, 0}, /* int    maxTexture3D[3]          */
    {0, 0, 0}, /* int    maxTexture3DAlt[3]       */
    0,         /* int    maxTextureCubemap        */
    {0, 0},    /* int    maxTexture1DLayered[2]   */
    {0, 0, 0}, /* int    maxTexture2DLayered[3]   */
    {0, 0},    /* int    maxTextureCubemapLayered[2] */
    0,         /* int    maxSurface1D             */
    {0, 0},    /* int    maxSurface2D[2]          */
    {0, 0, 0}, /* int    maxSurface3D[3]          */
    {0, 0},    /* int    maxSurface1DLayered[2]   */
    {0, 0, 0}, /* int    maxSurface2DLayered[3]   */
    0,         /* int    maxSurfaceCubemap        */
    {0, 0},    /* int    maxSurfaceCubemapLayered[2] */
    0,         /* size_t surfaceAlignment         */
    0,         /* int    concurrentKernels        */
    0,         /* int    ECCEnabled               */
    0,         /* int    pciBusID                 */
    0,         /* int    pciDeviceID              */
    0,         /* int    pciDomainID              */
    0,         /* int    tccDriver                */
    0,         /* int    asyncEngineCount         */
    0,         /* int    unifiedAddressing        */
    0,         /* int    memoryClockRate          */
    0,         /* int    memoryBusWidth           */
    0,         /* int    l2CacheSize              */
    0,         /* int    maxThreadsPerMultiProcessor */
    0,         /* int    streamPrioritiesSupported */
    0,         /* int    globalL1CacheSupported   */
    0,         /* int    localL1CacheSupported    */
    0,         /* size_t sharedMemPerMultiprocessor; */
    0,         /* int    regsPerMultiprocessor;   */
    0,         /* int    managedMemory            */
    0,         /* int    isMultiGpuBoard          */
    0,         /* int    multiGpuBoardGroupID     */
    0,         /* int    hostNativeAtomicSupported */
    0,         /* int    singleToDoublePrecisionPerfRatio */
    0,         /* int    pageableMemoryAccess     */
    0,         /* int    concurrentManagedAccess  */
    0,         /* int    computePreemptionSupported */
    0,         /* int    canUseHostPointerForRegisteredMem */
    0,         /* int    cooperativeLaunch */
    0,         /* int    cooperativeMultiDeviceLaunch */
    0,         /* size_t sharedMemPerBlockOptin */
};             /**< Empty device properties */

namespace et_runtime {

DeviceManager::DeviceManager() : active_device_(0), devices_(MAX_DEVICE_NUM) {}

shared_ptr<DeviceManager> getDeviceManager() {
  static shared_ptr<DeviceManager> deviceManager;

  if (!deviceManager) {
    deviceManager = make_shared<DeviceManager>();
  }
  return deviceManager;
}

ErrorOr<std::vector<DeviceInformation>> DeviceManager::enumerateDevices() {
  std::vector<DeviceInformation> info;
  switch (device::DeviceTarget::deviceToCreate()) {
  case device::DeviceTarget::TargetType::PCIe:
    return device::PCIeDevice::enumerateDevices();
    break;
  default:
    RTERROR << "Device enumeration is supported only for PCIE type devices \n";
    abort();
  }
  return etrtErrorDevicesUnavailable;
}

ErrorOr<int> DeviceManager::getDeviceCount() { return devices_.size(); }

ErrorOr<DeviceInformation> DeviceManager::getDeviceInformation(int device) {
  // FIXME SW-256
  assert(device == 0);
  auto prop = etrtDevicePropDontCare;
  // some values from CUDA Runtime 9.1 on GTX1060 (sm_61)
  prop.major = 6;
  prop.minor = 1;
  strcpy(prop.name, "Esperanto emulation of GeForce GTX 1060 6GB");
  prop.totalGlobalMem = 6371475456;
  prop.maxThreadsPerBlock = 1024;
  return prop;
}

ErrorOr<std::shared_ptr<Device>> DeviceManager::registerDevice(int device) {
  // FIXME Implement the real registration functionality that will depend on the
  // target device.
  if (static_cast<decltype(devices_)::size_type>(device) > devices_.size()) {
    return etrtErrorInvalidDevice;
  }

  devices_[device] = make_shared<Device>(device);

  return devices_[device];
}

ErrorOr<std::shared_ptr<Device>>
DeviceManager::getRegisteredDevice(int device) const {
  if (static_cast<decltype(devices_)::size_type>(device) > devices_.size()) {
    return etrtErrorInvalidDevice;
  }

  if (devices_[device].get() == nullptr) {
    return etrtErrorInvalidDevice;
  }

  return devices_[device];
}

ErrorOr<std::shared_ptr<Device>> DeviceManager::getActiveDevice() const {
  if (active_device_ > etrtErrorInvalidDevice) {
    return etrtErrorInvalidDevice;
  }
  return devices_[active_device_];
}

const char *DeviceManager::getDriverVersion() const {
  // FIXME Add support for queriing the device
  return "";
}

} // namespace et_runtime
