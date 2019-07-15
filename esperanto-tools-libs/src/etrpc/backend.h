#ifndef CARDEMU_BACKEND_H
#define CARDEMU_BACKEND_H

#include <stdint.h>
#include <string>
#include <sys/types.h>
#include <vector>
#include "Core/Device.h"

typedef enum {
  kIPIShutdown,
  kIPIInitMem,
  kIPIInitExecMem,
  kIPIWriteMem,
  kIPIReadMem,
  kIPIExecute,
  kIPISync,
  kIPIContinue
} CmdTypes;

typedef struct {
  long long addr;
  long long size;
} MemDescMsg;

typedef struct {
  uint64_t launch_pc;
  et_runtime::device::layer_dynamic_info params;
} ExecuteDescMsg;

class Backend {
public:
  Backend() : comm_socket_(-1), comm_fd_(-1), emu_pid_(-1) {}
  virtual ~Backend() {}

  virtual void defineMemory(void *memory_addr, size_t memory_size);
  virtual void defineExecMemory(void *memory_addr, size_t memory_size);
  virtual void writeMemory(void *memory_ptr, void *memory_addr,
                           size_t memory_size);
  virtual void readMemory(void *memory_ptr, void *memory_addr,
                          size_t memory_size);
  virtual void launch(uint64_t launch_pc, const et_runtime::device::layer_dynamic_info &params);
  virtual void executeCodeAndWait(uint64_t launch_pc, const et_runtime::device::layer_dynamic_info &params);
  virtual void shutdown(const char *work_dir);

  virtual void startup(const char *work_dir) = 0;
  virtual void boot(uint64_t init_pc, uint64_t trap_pc) = 0;
  void setupCommunicationSocket(const char *path);
  void acceptCommunicationSocket();

protected:
  virtual const char *getCommunicationFile() = 0;

  ssize_t readBytesFromFd(void *buf, size_t count);
  ssize_t writeBytesIntoFd(const void *buf, size_t count);

  void createEmuProcess(const char *path, const std::vector<std::string> &argv);

  int comm_socket_; // communication socket for accepting connections
  int comm_fd_;     // FD for data transfer
  int emu_pid_;     // PID of simulator
};

class BackendUbt : public Backend {
public:
  ~BackendUbt() {}

  void startup(const char *work_dir);
  void boot(uint64_t init_pc, uint64_t trap_pc);

protected:
  const char *getCommunicationFile();
};

/* FIXME disabled for now
class BackendVcs : public Backend
{
  public:
    ~BackendVcs() {}

    void startup(const char* work_dir);
    void boot(uint64_t init_pc, uint64_t trap_pc);

  protected:
    const char* getCommunicationFile();
};
*/

class BackendSysEmu : public Backend {
public:
  ~BackendSysEmu() {}

  void startup(const char *work_dir);
  void boot(uint64_t init_pc, uint64_t trap_pc);

protected:
  const char *getCommunicationFile();
};

#endif // CARDEMU_BACKEND_H
