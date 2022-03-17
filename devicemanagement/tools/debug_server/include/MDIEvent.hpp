#include <cstdint>
#include <MinionDebugInterface.hpp>
#include <RspConnection.hpp>
#include <RspPacket.hpp>

class MDIEvent {
 public:
  // Constructor and destructor
  /**
   * @brief Constructor
   */
  MDIEvent(MinionDebugInterface *mdi, RspPacket *pkt, RspConnection *rsp);
  ~MDIEvent();

  // Thread to pop async events from DM/device
  void deviceAsyncEventThread();
  void reportTargetHalted();

 private:
  
  // MDI Interface 
  MinionDebugInterface *mdi;

  //! RSP interface (which we create)
  RspConnection *rsp;

  //! RSP packet pointer.
  RspPacket *pkt;

  //! Data taken from the GDB 6.8 source. Only those we use defined here.
  enum TargetSignal { TARGET_SIGNAL_NONE = 0, TARGET_SIGNAL_TRAP = 5 };
 
   /**
   * @brief Verify if valid DMLib is loaded and available for access from MDI
   */
  int verifyDMLib();

};  // MDIEvent ()