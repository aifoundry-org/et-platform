//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "Core/Commands.h"
#include "Core/Device.h"
#include "Support/STLHelpers.h"
#include "etrt-bin.h"

#define INCLUDE_FOR_HOST
#include "../etlibdevice.h"
#include "../kernels/sys_inc.h"
#undef INCLUDE_FOR_HOST

//#include "cpu_algo.h"
#include "demangle.h"

using namespace et_runtime;

#if 0
static bool static_kernels = false;
#endif

void EtActionEvent::execute(Device *device) {
  std::lock_guard<std::mutex> lk(observer_mutex);
  executed = true;
  observer_cond_var.notify_all();
}

void EtActionEvent::observerWait() {
  std::unique_lock<std::mutex> lk(observer_mutex);
  observer_cond_var.wait(lk, [this] { return this->executed; });
  // EtAction::decRefCounter(this);
}

void EtActionConfigure::execute(Device *device) {
  auto &target_device = device->getTargetDevice();
  target_device.defineDevMem(LAUNCH_PARAMS_AREA_BASE, LAUNCH_PARAMS_AREA_SIZE,
                             false);

  target_device.defineDevMem(BLOCK_SHARED_REGION,
                             BLOCK_SHARED_REGION_TOTAL_SIZE, false);

  target_device.defineDevMem(STACK_REGION, STACK_REGION_TOTAL_SIZE << 3, false);

  target_device.defineDevMem(0x8000100000, 0x100000, false);
  target_device.defineDevMem(0x8000300000, 0x108000, false);
  target_device.defineDevMem(0x8000408000, 0x108000, false);
  target_device.defineDevMem(0x8200000000, 64, false);
  target_device.defineDevMem(0x8000600000, 64, false);

  // const void *kernels_file_p = gEtKernelsElf;
  // size_t kernels_file_size = sizeof(gEtKernelsElf);
  const auto &bootrom = device->getBootRom();
  const void *bootrom_file_p = reinterpret_cast<const void *>(bootrom.data());
  size_t bootrom_file_size = bootrom.size();

  // target_device.defineDevMem( RAM_MEMORY_REGION, align_up(kernels_file_size,
  // 0x10000), true); cpWriteDevMem( RAM_MEMORY_REGION,
  // kernels_file_size, kernels_file_p);

  target_device.defineDevMem(BOOTROM_START_IP,
                             align_up(bootrom_file_size, 0x1000), true);
  target_device.writeDevMem(BOOTROM_START_IP, bootrom_file_size,
                            bootrom_file_p);

// FIXME deprecate the following eventually
#if 1

  static const long ETSOC_init  = 0x000000810000600c;
  static const long ETSOC_mtrap = 0x0000008100007000;

  {
    struct BootromInitDescr_t {
      uint64_t init_pc;
      uint64_t trap_pc;
    } descr;

    descr.init_pc = ETSOC_init;
    descr.trap_pc = ETSOC_mtrap;

    target_device.writeDevMem(LAUNCH_PARAMS_AREA_BASE, sizeof(descr), &descr);
  }

  target_device.boot(ETSOC_init, ETSOC_mtrap);
#else
  target_device.boot(0, 0);
#endif

  target_device.defineDevMem((uintptr_t)devMemRegionPtr, devMemRegionSize,
                             false);
  target_device.defineDevMem((uintptr_t)kernelsDevMemRegionPtr,
                             kernelsDevMemRegionSize, true);
}

void EtActionRead::execute(Device *device) {
  auto &target_device = device->getTargetDevice();
  target_device.readDevMem((uintptr_t)srcDevPtr, count, dstHostPtr);
}

void EtActionWrite::execute(Device *device) {
  auto &target_device = device->getTargetDevice();

  target_device.writeDevMem((uintptr_t)dstDevPtr, count, srcHostPtr);
}

void EtActionLaunch::execute(Device *device) {
  auto &target_device = device->getTargetDevice();
  fprintf(stderr,
          "Going to execute kernel {0x%lx} %s [%s] grid_dim=(%d,%d,%d) "
          "block_dim=(%d,%d,%d)\n",
          kernel_pc, kernel_name.c_str(), demangle(kernel_name).c_str(),
          gridDim.x, gridDim.y, gridDim.z, blockDim.x, blockDim.y, blockDim.z);
// FIXME remove the following
#if 0
  if (kernel_pc == 0) {
    // this is for builtin kernels (calling kernel by name, i.e. kernel_pc == 0)
    kernel_pc = getBuiltinKernelPcByName(kernel_name);
  }

  if (!card_proxy) // i.e. local mode
  {
    target_device.cpuLaunch(kernel_name, kernel_pc, args_buff);
    return;
  }
#endif

  {
    size_t args_size = args_buff.size();
    size_t params_size = sizeof(LaunchParams_t) + args_size;
    assert(params_size < LAUNCH_PARAMS_AREA_SIZE);

    std::vector<uint8_t> params(params_size);
    LaunchParams_t *params_p = (LaunchParams_t *)&params[0];

    params_p->state.grid_size_x = gridDim.x;
    params_p->state.grid_size_y = gridDim.y;
    params_p->state.grid_size_z = gridDim.z;
    params_p->state.block_size_x = blockDim.x;
    params_p->state.block_size_y = blockDim.y;
    params_p->state.block_size_z = blockDim.z;

    params_p->kernel_pc = kernel_pc;
    params_p->args_size = args_size;

    memcpy(&params_p->args[0], &args_buff[0], args_size);

    target_device.writeDevMem(LAUNCH_PARAMS_AREA_BASE, params_size, params_p);
  }

#if 0
  // b4c hack
  static const long ETSOC_launch = 0x0000008100006038;
  if (static_kernels)
    target_device.launch( ETSOC_launch); // b4c - precompiled kernels
  else
#endif
  target_device.launch(kernel_pc); // ETSOC backend - JIT, not covered by b4c
}
