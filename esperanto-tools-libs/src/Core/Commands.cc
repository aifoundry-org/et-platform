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
#include "et_bootrom.h"
#include "etrt-bin.h"

#define INCLUDE_FOR_HOST
#include "../kernels/etlibdevice.h"
#include "../kernels/sys_inc.h"
#undef INCLUDE_FOR_HOST

#include "cpu_algo.h"
#include "demangle.h"
#include "et_bootrom.h"
#include "kernels_offsets.h"

// clang-format off
// et-rpc is an external dependency to be deprecated
// unfortunately the et-card-proxy.h header is not self
// contained and misisng includes
#include <stddef.h>
#include <stdint.h>
#include "et-rpc/et-card-proxy.h"
// clang-format on

using namespace et_runtime;

static bool static_kernels = false;

void EtActionEvent::execute(CardProxy *card_proxy) {
  std::lock_guard<std::mutex> lk(observer_mutex);
  executed = true;
  observer_cond_var.notify_all();
}

void EtActionEvent::observerWait() {
  std::unique_lock<std::mutex> lk(observer_mutex);
  observer_cond_var.wait(lk, [this] { return this->executed; });
  // EtAction::decRefCounter(this);
}

void EtActionConfigure::execute(CardProxy *card_proxy) {
  if (!card_proxy) // i.e. local mode
  {
    res_is_local_mode = true;
    return;
  }

  cpDefineDevMem(card_proxy, LAUNCH_PARAMS_AREA_BASE, LAUNCH_PARAMS_AREA_SIZE,
                 false);

  cpDefineDevMem(card_proxy, BLOCK_SHARED_REGION,
                 BLOCK_SHARED_REGION_TOTAL_SIZE, false);

  cpDefineDevMem(card_proxy, STACK_REGION, STACK_REGION_TOTAL_SIZE << 3, false);

  // const void *kernels_file_p = gEtKernelsElf;
  // size_t kernels_file_size = sizeof(gEtKernelsElf);
  const void *bootrom_file_p = *(etrtGetEtBootrom());
  size_t bootrom_file_size = *(etrtGetEtBootromSize());

  // cpDefineDevMem(card_proxy, RAM_MEMORY_REGION, align_up(kernels_file_size,
  // 0x10000), true); cpWriteDevMem(card_proxy, RAM_MEMORY_REGION,
  // kernels_file_size, kernels_file_p);

  cpDefineDevMem(card_proxy, BOOTROM_START_IP,
                 align_up(bootrom_file_size, 0x1000), true);
  cpWriteDevMem(card_proxy, BOOTROM_START_IP, bootrom_file_size,
                bootrom_file_p);

  {
    struct BootromInitDescr_t {
      uint64_t init_pc;
      uint64_t trap_pc;
    } descr;

    descr.init_pc = ETSOC_init;
    descr.trap_pc = ETSOC_mtrap;

    cpWriteDevMem(card_proxy, LAUNCH_PARAMS_AREA_BASE, sizeof(descr), &descr);
  }

  cpBoot(card_proxy, ETSOC_init, ETSOC_mtrap);

  cpDefineDevMem(card_proxy, (uintptr_t)devMemRegionPtr, devMemRegionSize,
                 false);
  cpDefineDevMem(card_proxy, (uintptr_t)kernelsDevMemRegionPtr,
                 kernelsDevMemRegionSize, true);
}

void EtActionRead::execute(CardProxy *card_proxy) {
  if (!card_proxy) // i.e. local mode
  {
    memcpy(dstHostPtr, srcDevPtr, count);
    return;
  }

  cpReadDevMem(card_proxy, (uintptr_t)srcDevPtr, count, dstHostPtr);
}

void EtActionWrite::execute(CardProxy *card_proxy) {
  if (!card_proxy) // i.e. local mode
  {
    memcpy(dstDevPtr, srcHostPtr, count);
    return;
  }

  cpWriteDevMem(card_proxy, (uintptr_t)dstDevPtr, count, srcHostPtr);
}

void EtActionLaunch::execute(CardProxy *card_proxy) {
  fprintf(stderr,
          "Going to execute kernel {0x%lx} %s [%s] grid_dim=(%d,%d,%d) "
          "block_dim=(%d,%d,%d)\n",
          kernel_pc, kernel_name.c_str(), demangle(kernel_name).c_str(),
          gridDim.x, gridDim.y, gridDim.z, blockDim.x, blockDim.y, blockDim.z);

  if (kernel_pc == 0) {
    // this is for builtin kernels (calling kernel by name, i.e. kernel_pc == 0)
    kernel_pc = getBuiltinKernelPcByName(kernel_name);
  }

  if (!card_proxy) // i.e. local mode
  {
    cpuLaunch(kernel_name, kernel_pc, args_buff);
    return;
  }

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

    cpWriteDevMem(card_proxy, LAUNCH_PARAMS_AREA_BASE, params_size, params_p);
  }

  // b4c hack
  if (static_kernels)
    cpLaunch(card_proxy, ETSOC_launch); // b4c - precompiled kernels
  else
    cpLaunch(card_proxy, kernel_pc); // ETSOC backend - JIT, not covered by b4c
}
