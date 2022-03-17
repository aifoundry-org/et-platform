#include <cstring>
#include <chrono>
#include <MDIEvent.hpp>
#include <MiscUtils.hpp>
#include <iomanip>
#include <iostream>
#include <thread>

using std::cout;
using std::cerr;
using std::dec;
using std::endl;
using std::hex;

MDIEvent::MDIEvent(MinionDebugInterface *mdi, RspPacket *pkt, RspConnection *rsp) {
   MDIEvent::mdi = mdi;
   MDIEvent::pkt = pkt;
   MDIEvent::rsp = rsp;
}  // MDIEvent ()

MDIEvent::~MDIEvent() {
}  // ~MDIEvent

int MDIEvent::verifyDMLib() {

    auto retval = 0;

    if (!(mdi->devLayer_.get()) || mdi->devLayer_.get() == nullptr) {
        DV_LOG(INFO) << "Device Layer pointer is null!" << std::endl;
        retval = -EAGAIN;
    }

    if (!mdi->dmi_) {
        DV_LOG(INFO) << "Device Management instance is null!" << std::endl;
        retval = -EAGAIN;
    }

    return retval;
}

//-----------------------------------------------------------------------------
//! Thread to pop  async events from device and send it to GDB client
//-----------------------------------------------------------------------------
void MDIEvent::deviceAsyncEventThread() {

    DV_LOG(INFO) << "deviceAsyncEventThread running .." << std::endl;

    auto retval = verifyDMLib();

    if (retval != 0) {
        DV_LOG(INFO) << "Failed to verify the DM lib: " << std::endl;
        DV_LOG(INFO) << "Failed to start the async event thread.." << std::endl;
    }

    DeviceManagement& dm = mdi->dmi_(mdi->devLayer_.get());
    std::vector<std::byte> response;

    while (!mdi->shouldStopServer())
    {
        // Wait for debug async events
#if MINION_DEBUG_INTERFACE
        
        bool event_occured = dm.getEvent(mdi->device_idx, response, mdi->bp_timeout);

        if(event_occured){
            DV_LOG(INFO) << "deviceAsyncEventThread event_occured .." << std::endl;
            auto event = reinterpret_cast<const device_mgmt_mdi_bp_event_t*>(response.data());
   
            switch(event->event_type){
                case MDI_EVENT_TYPE_BP_HALT_SUCCESS:
                  DV_LOG(INFO) << "Event Type: MDI_EVENT_TYPE_BP_HALT_SUCCESS" << std::endl;
                  reportTargetHalted();
                  break;

                 case MDI_EVENT_TYPE_BP_HALT_FAILED:
                  DV_LOG(INFO) << "Event Type: MDI_EVENT_TYPE_BP_HALT_FAILED" << std::endl;
                  break;

                  default:
                  DV_LOG(INFO) << "Unsuported event reported" << std::endl;
                  break;
            }
        }
#endif
    }

  DV_LOG(INFO) << "deviceAsyncEventThread exiting .." << std::endl;

}  // deviceAsyncEventThread ()


void MDIEvent::reportTargetHalted() {
    
    // Select the Hart 
    mdi->selectHart(mdi->shire_id, mdi->thread_mask);

    // Set the internal state (used by debug server) of the tgt as Halted.
    mdi->tgt_state = TGT_HALTED;

    // Construct a signal received packet
    pkt->data[0] = 'S';
    pkt->data[1] = Utils::hex2Char(TARGET_SIGNAL_TRAP >> 4);
    pkt->data[2] = Utils::hex2Char(TARGET_SIGNAL_TRAP % 16);
    pkt->data[3] = '\0';
    pkt->setLen(strlen(pkt->data));

    rsp->putPkt(pkt);

}  // reportTargetHalted ()