//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include <dlfcn.h>

#include "utils.h"
#include "string.h"
#include "MinionDebugInterface.hpp"
#include <cstdio>
#include <iostream>

// Constructor and Destructor

MinionDebugInterface::MinionDebugInterface(uint64_t shire_mask, uint64_t thread_mask) {

    this->handle_ = dlopen("libDM.so", RTLD_LAZY);
    this->devLayer_ = IDeviceLayer::createPcieDeviceLayer(false, true);
    this->dmi_ = reinterpret_cast<getDM_t>(dlsym(this->handle_, "getInstance"));

}  // MinionDebugInterface ()

MinionDebugInterface::~MinionDebugInterface() {

    if (this->handle_ != nullptr) {
      dlclose(this->handle_);
    }

}  // ~MinionDebugInterface

// libDM Helpers

int MinionDebugInterface::verifyDMLib() {

    auto retval = 0;

    if (!(this->devLayer_.get()) || this->devLayer_.get() == nullptr) {
        DV_LOG(INFO) << "Device Layer pointer is null!" << std::endl;
        retval = -EAGAIN;
    }

    if (!this->dmi_) {
        DV_LOG(INFO) << "Device Management instance is null!" << std::endl;
        retval = -EAGAIN;
    }

    return retval;
}

int MinionDebugInterface::invokeDmServiceRequest(uint8_t code, const char* input_buff,
    const uint32_t input_size, char* output_buff, const uint32_t output_size) {

  auto retval = verifyDMLib();

  if (retval != verifyDMLib()) {
    DV_LOG(INFO) << "Failed to verify the DM lib: " << std::endl;
    return retval;
  }

  DeviceManagement& dm = this->dmi_(devLayer_.get());

  auto hst_latency = std::make_unique<uint32_t>();
  auto dev_latency = std::make_unique<uint64_t>();

  retval = dm.serviceRequest(0, code, input_buff, input_size, output_buff,
            output_size, hst_latency.get(), dev_latency.get(), 70000);

  if (retval != DM_STATUS_SUCCESS) {
    DV_LOG(INFO) << "Service request failed with return code: " << std::endl;
    return retval;
  }

  return 0;
}

//Methods

void MinionDebugInterface::kill() {

    DV_LOG(INFO) << "MDI::kill.." << std::endl;
}

void MinionDebugInterface::reset() {

    DV_LOG(INFO) << "MDI::reset.." << std::endl;
}

void MinionDebugInterface::stall() {

    DV_LOG(INFO) << "MDI::stall.." << std::endl;
    // By default this halts the Hart0.
    // this->MinionDebugInterface::haltHart();

}

void MinionDebugInterface::closeMDI() {

    DV_LOG(INFO) << "MDI::closeMDI.." << std::endl;
    this->MinionDebugInterface::~MinionDebugInterface();
}

void MinionDebugInterface::unstall() {

    DV_LOG(INFO) << "MDI::unstall.." << std::endl;
    this->resumeHart();
    this->unselectHart(0,1);
}

bool MinionDebugInterface::isStalled() {

    DV_LOG(INFO) << "MDI::isStalled.." << std::endl;
    return true;
}

void MinionDebugInterface::step() {

    struct mdi_ss_control_t mdi_cmd;
    uint32_t status;

    mdi_cmd.shire_mask = 0; //TODO, logic to set current shire mask here
    mdi_cmd.thread_mask = 0; //TODO, logic to set current thread mast here
    mdi_cmd.flags = 0; //TODO, logic to set flags for single stepping

#if ENABLE_DEBUG_LOGS
    DV_LOG(INFO) << "MDI::step.." << std::endl;
#endif

    auto retval = invokeDmServiceRequest(DM_CMD_MDI_ENABLE_SINGLE_STEP,
        (char*) &mdi_cmd, sizeof(struct mdi_bp_control_t),
        (char*) &status, sizeof(uint32_t));

#if ENABLE_DEBUG_LOGS
    DV_LOG(INFO) << "MDI::insertBreakpoint.." << std::endl;
#endif

}

void MinionDebugInterface::insertBreakpoint(uint64_t addr) {

    struct mdi_bp_control_t mdi_cmd;
    uint32_t status;

    mdi_cmd.hart_id = this->current_hart_; // assign current hart ID
    mdi_cmd.bp_address = addr;
    mdi_cmd.mode = 0; //TODO, fix logic here, assign minion mode here
    mdi_cmd.flags = 0; //TODO, fix logic here, set flags to indicate set/insert break point

    auto retval = invokeDmServiceRequest(DM_CMD_MDI_SET_BREAKPOINT,
        (char*) &mdi_cmd, sizeof(struct mdi_bp_control_t),
        (char*) &status, sizeof(uint32_t));

    DV_LOG(INFO) << "MDI::insertBreakpoint.." << std::endl;
}

void MinionDebugInterface::removeBreakpoint(uint64_t addr) {

    struct mdi_bp_control_t mdi_cmd;
    uint32_t status;

    mdi_cmd.hart_id = this->current_hart_; // assign current hart ID
    mdi_cmd.bp_address = addr;
    mdi_cmd.mode = 0; //TODO, fix logic here, assign minion mode here
    mdi_cmd.flags = 0; //TODO, fix logic here, set flags to indicate unset/remove break point

    auto retval = invokeDmServiceRequest(DM_CMD_MDI_UNSET_BREAKPOINT,
        (char*) &mdi_cmd, sizeof(struct mdi_bp_control_t),
        (char*) &status, sizeof(uint32_t));

    DV_LOG(INFO) << "MDI::removeBreakpoint.." << std::endl;
}

uint64_t MinionDebugInterface::selectHart(std::uint64_t shire_id, std::uint64_t thread_mask) {

    struct mdi_hart_selection_t mdi_cmd;
    const uint32_t output_size = sizeof(uint64_t);
    char output_buff[output_size] = {0};
    mdi_cmd.shire_id = shire_id;
    mdi_cmd.thread_mask = thread_mask;

#if ENABLE_DEBUG_LOGS
    DV_LOG(INFO) << "MDI::selectHart ->" << "Shire ID: " << std::hex << mdi_cmd.shire_id << "Thread Mask: " << std::hex << mdi_cmd.thread_mask << std::endl;
#endif

    auto retval = invokeDmServiceRequest(DM_CMD_MDI_SELECT_HART,
        (char*) &mdi_cmd, sizeof(struct mdi_hart_selection_t),
        (char*) &output_buff, sizeof(uint64_t));

#if ENABLE_DEBUG_LOGS
    DV_LOG(INFO) << "Select Hart Status: " << std::hex << retval << std::endl;
#endif
}

uint64_t MinionDebugInterface::unselectHart(std::uint64_t shire_id, std::uint64_t thread_mask) {

    struct mdi_hart_selection_t mdi_cmd;
    const uint32_t output_size = sizeof(uint64_t);
    char output_buff[output_size] = {0};
    mdi_cmd.shire_id = shire_id;
    mdi_cmd.thread_mask = thread_mask;

#if ENABLE_DEBUG_LOGS
    DV_LOG(INFO) << "MDI::unselectHart ->" << "Shire ID: " << std::hex << mdi_cmd.shire_id << "Thread Mask: " << std::hex << mdi_cmd.thread_mask << std::endl;
#endif

    auto retval = invokeDmServiceRequest(DM_CMD_MDI_UNSELECT_HART,
        (char*) &mdi_cmd, sizeof(struct mdi_hart_selection_t),
        (char*) &output_buff, sizeof(uint64_t));

#if ENABLE_DEBUG_LOGS
    DV_LOG(INFO) << "UnSelect Hart Status: " << std::hex << retval << std::endl;
#endif

}

void MinionDebugInterface::haltHart() {

    struct mdi_hart_control_t mdi_cmd;
    const uint32_t output_size = sizeof(uint64_t);
    char output_buff[output_size] = {0};

#if ENABLE_DEBUG_LOGS
    DV_LOG(INFO) << "MDI::haltHart" << std::endl;
#endif

    auto retval = invokeDmServiceRequest(DM_CMD_MDI_HALT_HART,
        (char*) &mdi_cmd, sizeof(struct mdi_hart_control_t),
        (char*) &output_buff, sizeof(uint32_t));

#if ENABLE_DEBUG_LOGS
    DV_LOG(INFO) << "Halt Hart Complete" << std::endl;
#endif

}

void MinionDebugInterface::resumeHart() {

    struct mdi_hart_control_t mdi_cmd;
    const uint32_t output_size = sizeof(uint64_t);
    char output_buff[output_size] = {0};

#if ENABLE_DEBUG_LOGS
    DV_LOG(INFO) << "MDI::resumeHart" << std::endl;
#endif

    auto retval = invokeDmServiceRequest(DM_CMD_MDI_RESUME_HART,
        (char*) &mdi_cmd, sizeof(struct mdi_hart_control_t),
        (char*) &output_buff, sizeof(uint32_t));

#if ENABLE_DEBUG_LOGS
    DV_LOG(INFO) << "Resume Hart Complete" << std::endl;
#endif

}

uint64_t MinionDebugInterface::readReg(std::uint32_t num) {

    struct mdi_gpr_read_t mdi_cmd;
    uint64_t regval;

    mdi_cmd.hart_id = 0;
    mdi_cmd.gpr_index = num;

#if ENABLE_DEBUG_LOGS
    DV_LOG(INFO) << "MDI::readReg ->" << "Hart ID: " << std::hex << mdi_cmd.hart_id << " GPR Index: " << std::hex << mdi_cmd.gpr_index << std::endl;
#endif

    auto retval = invokeDmServiceRequest(DM_CMD_MDI_READ_GPR,
        (char*) &mdi_cmd, sizeof(struct mdi_gpr_read_t),
        (char*) &regval, sizeof(uint64_t));

#if ENABLE_DEBUG_LOGS
    DV_LOG(INFO) << "GPR Reg Value: " << std::hex << regval << std::endl;
#endif

    return regval;
}

uint64_t MinionDebugInterface::readCSRReg(std::uint32_t num) {

    struct mdi_csr_read_t mdi_cmd;
    uint64_t regval;

    mdi_cmd.hart_id = 0;
    mdi_cmd.csr_name = num;

#if ENABLE_DEBUG_LOGS
    DV_LOG(INFO) << "MDI::readReg ->" << "Hart ID: " << std::hex << mdi_cmd.hart_id << " CSR Index: " << std::hex << mdi_cmd.csr_name << std::endl;
#endif

    auto retval = invokeDmServiceRequest(DM_CMD_MDI_READ_CSR,
        (char*) &mdi_cmd, sizeof(struct mdi_csr_read_t),
        (char*) &regval, sizeof(uint64_t));

#if ENABLE_DEBUG_LOGS
    DV_LOG(INFO) << "CSR Reg Value: " << std::hex << regval << std::endl;
#endif

    return regval;
}

void MinionDebugInterface::writeReg(std::uint32_t num, uint64_t value) {

    struct mdi_gpr_write_t mdi_cmd;
    uint64_t dummy;

    mdi_cmd.hart_id = 0;
    mdi_cmd.data = value;
    mdi_cmd.gpr_index = num;

#if ENABLE_DEBUG_LOGS
    DV_LOG(INFO) << "MDI::writeReg ->" << " num: " << std::hex << num << "value: " << std::hex << value << std::endl;
    DV_LOG(INFO) << "MDI::writeReg ->" << " GPR Index: " << std::hex << mdi_cmd.gpr_index << "Data: " << std::hex << mdi_cmd.data << std::endl;
#endif

    auto retval = invokeDmServiceRequest(DM_CMD_MDI_WRITE_GPR,
        (char*) &mdi_cmd, sizeof(struct mdi_gpr_write_t),
        (char*) &dummy, sizeof(uint64_t));
}

int MinionDebugInterface::readMem(uint8_t *out, uint64_t addr, std::uint32_t len) {

    struct mdi_mem_read_t mdi_cmd_req;
    mdi_cmd_req.address = addr;
    mdi_cmd_req.size = len;
    const uint32_t output_size = 8;

#if ENABLE_DEBUG_LOGS
     DV_LOG(INFO) << "MDI::readMem" << " ADDRESS: " << std::hex << mdi_cmd_req.address << " LENGTH: " << std::hex << mdi_cmd_req.size << std::endl;
 #endif
    // TODO: Loop over length and check for retval status
    auto retval = invokeDmServiceRequest(DM_CMD_MDI_READ_MEM,
            (char*) &mdi_cmd_req, sizeof(mdi_cmd_req),
            (char*) out, output_size);

#if ENABLE_DEBUG_LOGS
    DV_LOG(INFO) << "MDI::readMem.. completed " << std::endl;
#endif

    return retval;
}

int MinionDebugInterface::writeMem(uint8_t *src, uint64_t addr, std::uint32_t len) {

    struct mdi_mem_write_t mdi_cmd;
    uint32_t status;

    mdi_cmd.address = addr;
    mdi_cmd.size = sizeof(uint64_t);

    // TODO:Loop needs to be fixed/optimized and check for writemem status

    //Check the length of data to be written to memory
    while((len) > sizeof(uint64_t)) {

    memcpy((char*)&mdi_cmd.data, src, sizeof(uint64_t));

    auto retval = invokeDmServiceRequest(DM_CMD_MDI_WRITE_MEM,
        (char*) &mdi_cmd, sizeof(mdi_cmd),
        (char*) &status, sizeof(uint64_t));

    /* Adjust the remaining length and address */
    len = len - sizeof(uint64_t);
    mdi_cmd.address += sizeof(uint64_t);

    }

    /* Write the remaining bytes of data */
    if(len > 0){

    memcpy((char*)&mdi_cmd.data, src, len);
    mdi_cmd.size = len;

    auto retval = invokeDmServiceRequest(DM_CMD_MDI_WRITE_MEM,
        (char*) &mdi_cmd, sizeof(mdi_cmd),
        (char*) &status, sizeof(uint64_t));
    }
}

uint32_t MinionDebugInterface::pcRegNum() {

    // return register refernce to pc in the method
    DV_LOG(INFO) << "MDI::pcRegNum.." << std::endl;
    return RISCV_PC_INDEX;
}

uint32_t MinionDebugInterface::nRegs() {

    return RISCV64_NUM_GPRS;
}

uint32_t MinionDebugInterface::wordSize() {

    // return register size for riscv64
    return RISCV64_REG_SIZE;
}

// Control Debug Server

void MinionDebugInterface::stopServer() {

    DV_LOG(INFO) << "MDI::stopServer.." << std::endl;
}

bool MinionDebugInterface::shouldStopServer() {

    DV_LOG(INFO) << "MDI::shouldStopServer.." << std::endl;
    return false;
}

bool MinionDebugInterface::isServerRunning() {

    DV_LOG(INFO) << "MDI::isServerRunning.." << std::endl;
    return true;
}

void MinionDebugInterface::setServerRunning(bool status) {

    DV_LOG(INFO) << "MDI::setServerRunning.." << std::endl;
}

//Target specific utilities

uint64_t MinionDebugInterface::htotl(uint64_t hostVal) {

    //DV_LOG(INFO) << "MDI::htotl.." << std::endl;
    return hostVal;
}

uint64_t MinionDebugInterface::ttohl(uint64_t targetVal) {

    //DV_LOG(INFO) << "MDI::ttohl.." << std::endl;
    return targetVal;
}