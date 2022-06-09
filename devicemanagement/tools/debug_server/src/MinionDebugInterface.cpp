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

MinionDebugInterface::MinionDebugInterface(uint8_t device_idx, uint64_t shire_id, uint64_t thread_mask, uint64_t bp_timeout) {
    this->handle_ = dlopen("libDM.so", RTLD_LAZY);
    this->devLayer_ = IDeviceLayer::createPcieDeviceLayer(false, true);
    this->dmi_ = reinterpret_cast<getDM_t>(dlsym(this->handle_, "getInstance"));
    this->device_idx = device_idx;
    this->shire_id = shire_id;
    this->thread_mask = thread_mask;
    this->bp_timeout = bp_timeout;
    this->current_hart_ = ((shire_id*NUM_HARTS_PER_SHIRE) + __builtin_ctzl(thread_mask));
    this->server_state = false;
    this->should_stop_server = false;
 
     this->tgt_state = TGT_RUNNING;
    /* TODO: Enable this once device fw is updated */
    // this->tgt_state =  this->MinionDebugInterface::hartStatus(); 

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
        DV_LOG(INFO) << "Service request failed with return code: " << std::hex << retval << std::endl;
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

    DV_LOG(INFO) << "MDI::stall.." << "Shire ID:" << this->shire_id << "Thread Mask:" << std::hex << this->thread_mask << std::endl;
    this->selectHart(this->shire_id, this->thread_mask);
    this->haltHart();
    this->tgt_state = TGT_HALTED;
}

void MinionDebugInterface::closeMDI() {

    DV_LOG(INFO) << "MDI::closeMDI.." << std::endl;
    this->MinionDebugInterface::~MinionDebugInterface();
}

void MinionDebugInterface::unstall() {

    DV_LOG(INFO) << "MDI::unstall.." << "Shire ID:" << this->shire_id << "Thread Mask:" << std::hex << this->thread_mask << std::endl;
    this->resumeHart();
    this->unselectHart(this->shire_id,this->thread_mask);
    this->tgt_state = TGT_RUNNING;
}

bool MinionDebugInterface::isStalled() {
    
    /* TODO: Enable this once device bl is updated */
    // this->tgt_state = mdi->hartStatus();

    //DV_LOG(INFO) << "MDI::isStalled.." << std::endl; 
    if(this->tgt_state != TGT_RUNNING){
        //DV_LOG(INFO) << "MDI::isStalled..true" << std::endl;
        return true;
    }else {
        //DV_LOG(INFO) << "MDI::isStalled..false" << std::endl;
        return false;
    }
}

void MinionDebugInterface::step() {

    struct mdi_ss_control_t mdi_cmd;
    uint32_t status;

    mdi_cmd.shire_mask = this->shire_id; 
    mdi_cmd.thread_mask = this->thread_mask; 
    mdi_cmd.flags = 0; //TODO, logic to set flags for single stepping

    DV_LOG(INFO) << "MDI::step.." << std::endl;

    auto retval = invokeDmServiceRequest(DM_CMD_MDI_ENABLE_SINGLE_STEP,
        (char*) &mdi_cmd, sizeof(struct mdi_bp_control_t),
        (char*) &status, sizeof(uint32_t));

    DV_LOG(INFO) << "MDI::step status:" << status << std::endl;

}

void MinionDebugInterface::insertBreakpoint(uint64_t addr) {

    struct mdi_bp_control_t mdi_cmd;
    uint32_t status;

    mdi_cmd.hart_id = this->current_hart_;
    mdi_cmd.bp_address = addr;
    mdi_cmd.mode = device_mgmt_api::PRIV_MASK_PRIV_UMODE;
    mdi_cmd.bp_event_wait_timeout = this->bp_timeout; 
    mdi_cmd.flags = 0; //TODO, fix logic here, set flags to indicate set/insert break point

    DV_LOG(INFO) << "MDI::insertBreakpoint Hart ID:" << mdi_cmd.hart_id << "BP addr:" << std::hex << mdi_cmd.bp_address << std::endl;

    auto retval = invokeDmServiceRequest(DM_CMD_MDI_SET_BREAKPOINT,
        (char*) &mdi_cmd, sizeof(struct mdi_bp_control_t),
        (char*) &status, sizeof(uint32_t));

    DV_LOG(INFO) << "MDI::insertBreakpoint complete status:" << status << std::endl;
}

void MinionDebugInterface::removeBreakpoint(uint64_t addr) {

    struct mdi_bp_control_t mdi_cmd;
    uint32_t status;

    mdi_cmd.hart_id = this->current_hart_; // assign current hart ID
    mdi_cmd.bp_address = addr;
    mdi_cmd.mode = 0; //TODO, fix logic here, assign minion mode here
    mdi_cmd.flags = 0; //TODO, fix logic here, set flags to indicate unset/remove break point

    DV_LOG(INFO) << "MDI::removeBreakpoint Hart ID:" << mdi_cmd.hart_id  << "BP addr:" << std::hex <<  mdi_cmd.bp_address << std::endl;

    auto retval = invokeDmServiceRequest(DM_CMD_MDI_UNSET_BREAKPOINT,
        (char*) &mdi_cmd, sizeof(struct mdi_bp_control_t),
        (char*) &status, sizeof(uint32_t));

    DV_LOG(INFO) << "MDI::removeBreakpoint complete status:" << status << std::endl;
}

uint64_t MinionDebugInterface::selectHart(std::uint64_t shire_id, std::uint64_t thread_mask) {

    struct mdi_hart_selection_t mdi_cmd;
    const uint32_t output_size = sizeof(uint64_t);
    char output_buff[output_size] = {0};
    mdi_cmd.shire_id = shire_id;
    mdi_cmd.thread_mask = thread_mask;

    DV_LOG(INFO) << "MDI::selectHart ->" << "Shire ID: " << mdi_cmd.shire_id << "Thread Mask: " << std::hex << mdi_cmd.thread_mask << std::endl;

    auto retval = invokeDmServiceRequest(DM_CMD_MDI_SELECT_HART,
        (char*) &mdi_cmd, sizeof(struct mdi_hart_selection_t),
        (char*) &output_buff, sizeof(uint64_t));

    DV_LOG(INFO) << "MDI::selectHart Status: " << std::hex << retval << std::endl;
}

uint64_t MinionDebugInterface::unselectHart(std::uint64_t shire_id, std::uint64_t thread_mask) {

    struct mdi_hart_selection_t mdi_cmd;
    const uint32_t output_size = sizeof(uint64_t);
    char output_buff[output_size] = {0};
    mdi_cmd.shire_id = shire_id;
    mdi_cmd.thread_mask = thread_mask;

    DV_LOG(INFO) << "MDI::unselectHart ->" << "Shire ID: " << mdi_cmd.shire_id << "Thread Mask: " << std::hex << mdi_cmd.thread_mask << std::endl;

    auto retval = invokeDmServiceRequest(DM_CMD_MDI_UNSELECT_HART,
        (char*) &mdi_cmd, sizeof(struct mdi_hart_selection_t),
        (char*) &output_buff, sizeof(uint64_t));

    DV_LOG(INFO) << "MDI::unselectHart Status: " << std::hex << retval << std::endl;
}

void MinionDebugInterface::haltHart() {

    struct mdi_hart_control_t mdi_cmd;
    const uint32_t output_size = sizeof(uint64_t);
    char output_buff[output_size] = {0};

    DV_LOG(INFO) << "MDI::haltHart" << std::endl;

    auto retval = invokeDmServiceRequest(DM_CMD_MDI_HALT_HART,
        (char*) &mdi_cmd, sizeof(struct mdi_hart_control_t),
        (char*) &output_buff, sizeof(uint32_t));

    DV_LOG(INFO) << "MDI::haltHart Complete" << std::endl;
}

void MinionDebugInterface::resumeHart() {

    struct mdi_hart_control_t mdi_cmd;
    const uint32_t output_size = sizeof(uint64_t);
    char output_buff[output_size] = {0};

    DV_LOG(INFO) << "MDI::resumeHart" << std::endl;

    auto retval = invokeDmServiceRequest(DM_CMD_MDI_RESUME_HART,
        (char*) &mdi_cmd, sizeof(struct mdi_hart_control_t),
        (char*) &output_buff, sizeof(uint32_t));

    DV_LOG(INFO) << "MDI::resumeHart Complete" << std::endl;
}

uint32_t MinionDebugInterface::hartStatus() {

    struct mdi_hart_control_t mdi_cmd;
    uint32_t hart_status;
    mdi_cmd.hart_id = this->current_hart_;

    DV_LOG(INFO) << "MDI::hartStatus" << std::endl;

    auto retval = invokeDmServiceRequest(DM_CMD_MDI_GET_HART_STATUS,
        (char*) &mdi_cmd, sizeof(struct mdi_hart_control_t),
        (char*) &hart_status, sizeof(uint32_t));

    DV_LOG(INFO) << "MDI::hartStatus :" << hart_status << std::endl;

    return hart_status;
}


uint64_t MinionDebugInterface::readReg(std::uint32_t num) {

   uint64_t regval=0;
   if(num < RISCV_PC_INDEX){
        struct mdi_gpr_read_t mdi_gpr_read;
        mdi_gpr_read.hart_id = this->current_hart_;    
        mdi_gpr_read.gpr_index = num;
 
        DV_LOG(INFO) << "Read GPR " << "Hart ID: " << mdi_gpr_read.hart_id << " GPR Index: " << mdi_gpr_read.gpr_index << std::endl;
 
        auto retval = invokeDmServiceRequest(DM_CMD_MDI_READ_GPR,
            (char*) &mdi_gpr_read, sizeof(struct mdi_gpr_read_t),
            (char*) &regval, sizeof(uint64_t));
 
        DV_LOG(INFO) << "GPR Reg Value: " << std::hex << regval << std::endl;
   } 
   else 
   {
        struct mdi_csr_read_t mdi_csr_read;
        mdi_csr_read.hart_id = this->current_hart_;
        mdi_csr_read.csr_name = num;
 
        DV_LOG(INFO) << "Read CSR " << "Hart ID: " <<  mdi_csr_read.hart_id << " CSR Index: " << mdi_csr_read.csr_name << std::endl;
 
        auto retval = invokeDmServiceRequest(DM_CMD_MDI_READ_CSR,
            (char*) &mdi_csr_read, sizeof(struct mdi_csr_read_t),
            (char*) &regval, sizeof(uint64_t));

        DV_LOG(INFO) << "CSR Reg Value: " << std::hex << regval << std::endl;
   }

   return regval;
}

void MinionDebugInterface::writeReg(std::uint32_t num, uint64_t value) {

    uint64_t dummy;
    uint64_t tgt_value;

    tgt_value = this->uint64BytesSwap(value);
    
    if(num < RISCV_PC_INDEX){

        DV_LOG(INFO) << "MDI:: writeReg GPR ->" << " Reg num: " << num << "value: " << std::hex << value << std::endl;
        struct mdi_gpr_write_t mdi_gpr_write;
        mdi_gpr_write.hart_id = this->current_hart_;
        mdi_gpr_write.data = tgt_value;
        mdi_gpr_write.gpr_index = num;

        auto retval = invokeDmServiceRequest(DM_CMD_MDI_WRITE_GPR,
        (char*) &mdi_gpr_write, sizeof(struct mdi_gpr_write_t),
        (char*) &dummy, sizeof(uint64_t));
    } else {

        DV_LOG(INFO) << "MDI:: writeReg CSR ->" << " Reg num: " << num << "value: " << std::hex << value << std::endl;
        struct mdi_csr_write_t mdi_csr_write;
        mdi_csr_write.hart_id = this->current_hart_;
        mdi_csr_write.data = tgt_value;
        mdi_csr_write.csr_name = num;

        auto retval = invokeDmServiceRequest(DM_CMD_MDI_WRITE_CSR,
        (char*) &mdi_csr_write, sizeof(struct mdi_csr_write_t),
        (char*) &dummy, sizeof(uint64_t));

    }
}


int MinionDebugInterface::readMem(uint8_t *out, uint64_t addr, std::uint32_t len) {
    struct mdi_mem_read_t mdi_cmd_req;
    mdi_cmd_req.address = addr;
    uint8_t access_type;
    uint64_t access_initiator; 
    getAttributesForAddr(addr, &access_initiator, &access_type);
    mdi_cmd_req.hart_id = access_initiator;
    mdi_cmd_req.access_type = access_type;
    uint32_t output_size;
    
    while (len >= MDI_MEM_READ_LENGTH_BYTES_8)
    {
        mdi_cmd_req.size = MDI_MEM_READ_LENGTH_BYTES_8;
        DV_LOG(INFO) << "8 byte read ->" << " ADDRESS: " << std::hex << mdi_cmd_req.address << " LENGTH: " << std::hex << mdi_cmd_req.size << std::endl;

        auto retval = invokeDmServiceRequest(DM_CMD_MDI_READ_MEM,
                (char*) &mdi_cmd_req, sizeof(mdi_cmd_req),
                (char*) out, mdi_cmd_req.size);

        if (retval != DM_STATUS_SUCCESS){
            return retval;
        }

        mdi_cmd_req.address += MDI_MEM_READ_LENGTH_BYTES_8;
        out += MDI_MEM_READ_LENGTH_BYTES_8;
        len -= MDI_MEM_READ_LENGTH_BYTES_8;
    }

    while (len >= MDI_MEM_READ_LENGTH_BYTES_4)
    {
        mdi_cmd_req.size = MDI_MEM_READ_LENGTH_BYTES_4;
        
        DV_LOG(INFO) << "4 byte read ->" << " ADDRESS: " << std::hex << mdi_cmd_req.address << " LENGTH: " << std::hex << mdi_cmd_req.size << std::endl;

        auto retval = invokeDmServiceRequest(DM_CMD_MDI_READ_MEM,
                (char*) &mdi_cmd_req, sizeof(mdi_cmd_req),
                (char*) out, mdi_cmd_req.size);

        if (retval != DM_STATUS_SUCCESS){
            return retval;
        }

        mdi_cmd_req.address += MDI_MEM_READ_LENGTH_BYTES_4;
        out += MDI_MEM_READ_LENGTH_BYTES_4;
        len -= MDI_MEM_READ_LENGTH_BYTES_4;
    }

    while (len >= MDI_MEM_READ_LENGTH_BYTES_2)
    {
        mdi_cmd_req.size = MDI_MEM_READ_LENGTH_BYTES_2;
        
        DV_LOG(INFO) << "2 byte read ->" << " ADDRESS: " << std::hex << mdi_cmd_req.address << " LENGTH: " << std::hex << mdi_cmd_req.size << std::endl;

        auto retval = invokeDmServiceRequest(DM_CMD_MDI_READ_MEM,
                (char*) &mdi_cmd_req, sizeof(mdi_cmd_req),
                (char*) out, mdi_cmd_req.size);

        if (retval != DM_STATUS_SUCCESS){
            return retval;
        }

        mdi_cmd_req.address += MDI_MEM_READ_LENGTH_BYTES_2;
        out += MDI_MEM_READ_LENGTH_BYTES_2;
        len -= MDI_MEM_READ_LENGTH_BYTES_2;
    }

    while (len)
    {
        mdi_cmd_req.size = MDI_MEM_READ_LENGTH_BYTES_1;
        
        DV_LOG(INFO) << "1 byte read ->" << " ADDRESS: " << std::hex << mdi_cmd_req.address << " LENGTH: " 
                     << std::hex << mdi_cmd_req.size << std::endl;

        auto retval = invokeDmServiceRequest(DM_CMD_MDI_READ_MEM, (char*) &mdi_cmd_req, 
                                             sizeof(mdi_cmd_req), (char*) out, mdi_cmd_req.size);

        if (retval != DM_STATUS_SUCCESS){
            return retval;
        }

        mdi_cmd_req.address += MDI_MEM_READ_LENGTH_BYTES_1;
        out += MDI_MEM_READ_LENGTH_BYTES_1;
        len -= MDI_MEM_READ_LENGTH_BYTES_1;
    }

    return 0;
}

int MinionDebugInterface::writeMem(uint8_t *src, uint64_t addr, std::uint32_t len) {

    uint64_t status=0;
    struct mdi_mem_write_t mdi_cmd;
    mdi_cmd.address = addr;

     DV_LOG(INFO) << "MDI::writeMem" << " address: 0x" << std::hex << mdi_cmd.address << " len: " << std::hex << len << std::endl;

    /* If the addresses are 64-bit aligned */
    if (!((mdi_cmd.address) & 0x7))
    {
        while (len >= MDI_MEM_WRITE_LENGTH_BYTES_8)
        {

            DV_LOG(INFO) << "64 bit aligned" << " address: 0x" << std::hex << mdi_cmd.address << " len: " << std::hex << len << std::endl;
            memcpy((char*)&mdi_cmd.data, src, sizeof(uint64_t));
            mdi_cmd.size = sizeof(uint64_t);
            auto retval = invokeDmServiceRequest(DM_CMD_MDI_WRITE_MEM, (char*) &mdi_cmd, sizeof(mdi_cmd),
                                                (char*) &status, sizeof(uint64_t));
            DV_LOG(INFO) << "DM retval:" << std::hex << retval << " status: " << std::hex << status << std::endl;
            if (retval != DM_STATUS_SUCCESS){
                return retval;
            }

            if (status != DM_STATUS_SUCCESS) {
                return status;
            }
            mdi_cmd.address += MDI_MEM_WRITE_LENGTH_BYTES_8;
            len -= MDI_MEM_WRITE_LENGTH_BYTES_8;
        }
    }

    /* If the addresses are 32-bit aligned */
    if (!((mdi_cmd.address) & 0x3))
    {
        while (len >= MDI_MEM_WRITE_LENGTH_BYTES_4)
        {

            DV_LOG(INFO) << "32 bit aligned" << " address: 0x" << std::hex << mdi_cmd.address << " len: " << std::hex << len << std::endl;
            memcpy((char*)&mdi_cmd.data, src, sizeof(uint32_t));
            mdi_cmd.size = sizeof(uint32_t);
            auto retval = invokeDmServiceRequest(DM_CMD_MDI_WRITE_MEM, (char*) &mdi_cmd, sizeof(mdi_cmd),
                                                (char*) &status, sizeof(uint64_t));
            DV_LOG(INFO) << "DM retval:" << std::hex << retval << " status: " << std::hex << status << std::endl;
            if (retval != DM_STATUS_SUCCESS){
                return retval;
            }

            if (status != DM_STATUS_SUCCESS) {
                return status;
            }
            mdi_cmd.address += MDI_MEM_WRITE_LENGTH_BYTES_4;
            len -= MDI_MEM_WRITE_LENGTH_BYTES_4;
        }
    }
    /* If the addresses are 16-bit aligned */
    else if (!((mdi_cmd.address) & 0x1))
    {
        while (len >= MDI_MEM_WRITE_LENGTH_BYTES_2)
        {

            DV_LOG(INFO) << "16 bit aligned writeMem" << " address: 0x" << std::hex << mdi_cmd.address << " len: " << std::hex << len << std::endl;
            memcpy((char*)&mdi_cmd.data, src, sizeof(uint16_t));
            mdi_cmd.size = sizeof(uint16_t);
            auto retval = invokeDmServiceRequest(DM_CMD_MDI_WRITE_MEM, (char*) &mdi_cmd, sizeof(mdi_cmd),
                                                (char*) &status, sizeof(uint64_t));
            DV_LOG(INFO) << "DM retval:" << std::hex << retval << " status: " << std::hex << status << std::endl;
            if (retval != DM_STATUS_SUCCESS){
                return retval;
            }

            if (status != DM_STATUS_SUCCESS) {
                return status;
            }

            mdi_cmd.address += MDI_MEM_WRITE_LENGTH_BYTES_2;
            len -= MDI_MEM_WRITE_LENGTH_BYTES_2;
        }
    }

    /* Write byte aligned data (if any) */
    while (len)
    {

        DV_LOG(INFO) << "byte aligned" << " address: 0x" << std::hex << mdi_cmd.address << " len: " << std::hex << len << std::endl;
        memcpy((char*)&mdi_cmd.data, src, sizeof(uint8_t));
        mdi_cmd.size = sizeof(uint8_t);
        auto retval = invokeDmServiceRequest(DM_CMD_MDI_WRITE_MEM, (char*) &mdi_cmd, sizeof(mdi_cmd),
                                            (char*) &status, sizeof(uint64_t));
        DV_LOG(INFO) << "DM retval:" << std::hex << retval << " status: " << std::hex << status << std::endl;

        if (retval != DM_STATUS_SUCCESS){
            return retval;
        }

        if (status != DM_STATUS_SUCCESS) {
            return status;
        }
        
        mdi_cmd.address += MDI_MEM_WRITE_LENGTH_BYTES_1;
        len -= MDI_MEM_WRITE_LENGTH_BYTES_1;
    }

    DV_LOG(INFO) << "MDI::writeMem completed.." << std::endl;

    return 0;
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
    this->should_stop_server = true;
}

bool MinionDebugInterface::shouldStopServer() {

    //DV_LOG(INFO) << "MDI::shouldStopServer.." << std::endl;
    return this->should_stop_server;
}

bool MinionDebugInterface::isServerRunning() {

    DV_LOG(INFO) << "MDI::isServerRunning.." << std::endl;
    return this->server_state;
}

void MinionDebugInterface::setServerRunning(bool status) {

    DV_LOG(INFO) << "MDI::setServerRunning.." << status << std::endl;
    this->server_state = status;
}

//Target specific utilities

uint64_t MinionDebugInterface::htotl(uint64_t hostVal) {

    //DV_LOG(INFO) << "MDI::htotl.." << std::endl;
    return hostVal;
}

uint64_t MinionDebugInterface::getShireID() {

    return this->shire_id;
}

uint64_t MinionDebugInterface::getThreadMask() {

    return this->thread_mask;
}


uint64_t MinionDebugInterface::ttohl(uint64_t targetVal) {

    //DV_LOG(INFO) << "MDI::ttohl.." << std::endl;
    return targetVal;
}

uint64_t MinionDebugInterface::uint64BytesSwap(uint64_t val)
{
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL ) | ((val >> 8) & 0x00FF00FF00FF00FFULL );
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL ) | ((val >> 16) & 0x0000FFFF0000FFFFULL );
    return (val << 32) | (val >> 32);
}
