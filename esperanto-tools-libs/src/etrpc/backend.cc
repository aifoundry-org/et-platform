#include "backend.h"
#include "Support/Logging.h"

#include <assert.h>
#include <fcntl.h>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

ssize_t Backend::readBytesFromFd(void *buf, size_t count) {
  //  RTINFO << "Backend Read bytes: " << count;
  if (count == 0) {
    return count;
  }
  size_t n_bytes_read = 0;
  do {
    ssize_t r = TEMP_FAILURE_RETRY(
        read(comm_fd_, (char *)buf + n_bytes_read, count - n_bytes_read));
    if (r <= 0) {
      assert(!"Not all bytes read");
      return -1;
    }
    n_bytes_read += r;
  } while (n_bytes_read < count);

  assert(n_bytes_read == count);
  return n_bytes_read;
}

ssize_t Backend::writeBytesIntoFd(const void *buf, size_t count) {
  //  RTINFO << "Backend write bytes: " << count;
  if (count == 0) {
    return count;
  }
  size_t n_bytes_written = 0;
  do {
    ssize_t w = TEMP_FAILURE_RETRY(write(
        comm_fd_, (char *)buf + n_bytes_written, count - n_bytes_written));
    if (w <= 0) {
      assert(!"Not all bytes written");
      return -1;
    }
    n_bytes_written += w;
  } while (n_bytes_written < count);

  assert(n_bytes_written == count);
  return n_bytes_written;
}

void Backend::setupCommunicationSocket(const char *path) {
  comm_socket_ = socket(AF_UNIX, SOCK_STREAM, 0);

  assert(comm_socket_ >= 0);

  struct sockaddr_un un_addr;
  int res;

  memset(&un_addr, 0, sizeof(un_addr));
  un_addr.sun_family = AF_UNIX;
  strcpy(un_addr.sun_path, path);
  res = bind(comm_socket_, (struct sockaddr *)&un_addr, sizeof(un_addr));

  assert(res == 0);

  listen(comm_socket_, 1);
}

void Backend::acceptCommunicationSocket() {
  comm_fd_ = accept(comm_socket_, 0, 0);

  assert(comm_fd_ >= 0);
}

void Backend::createEmuProcess(const char *path,
                               const std::vector<std::string> &argv) {
  // prepare C-style argv (wrapped however by std::unique_ptr) with last
  // additional NULL
  std::unique_ptr<const char *[]> c_argv(new const char *[argv.size() + 1]);
  for (size_t i = 0; i < argv.size(); i++) {
    c_argv[i] = argv[i].c_str();
  }
  c_argv[argv.size()] = NULL;

  // pipe used to report error from execve call in child process
  int error_report_pipe_fd[2];
  pipe2(error_report_pipe_fd, O_CLOEXEC);

  emu_pid_ = fork();

  if (emu_pid_ == 0) {
    // child
    close(error_report_pipe_fd[0]);
    execv(path, const_cast<char *const *>(c_argv.get()));
    int errno_val = errno;

    write(error_report_pipe_fd[1], &errno_val, sizeof(int));
    exit(EXIT_FAILURE);
  }
  // parent
  close(error_report_pipe_fd[1]);
  int exec_errno = 0;
  if (TEMP_FAILURE_RETRY(
          read(error_report_pipe_fd[0], &exec_errno, sizeof(int))) != 0) {
    fprintf(stderr, "Failed to start emulator: %s (%s)\n", path,
            strerror(exec_errno));
    exit(EXIT_FAILURE);
  }
  close(error_report_pipe_fd[0]);
}

void Backend::defineMemory(void *memory_addr, size_t memory_size) {
  assert(comm_fd_ >= 0);

  MemDescMsg mem_desc;
  char cmd = kIPIInitMem;
  mem_desc.addr = reinterpret_cast<long long>(memory_addr);
  mem_desc.size = memory_size;

  writeBytesIntoFd(&cmd, 1);
  writeBytesIntoFd(&mem_desc, sizeof(mem_desc));
}

void Backend::defineExecMemory(void *memory_addr, size_t memory_size) {
  assert(comm_fd_ >= 0);

  MemDescMsg mem_desc;
  char cmd = kIPIInitExecMem;
  mem_desc.addr = reinterpret_cast<long long>(memory_addr);
  mem_desc.size = memory_size;

  writeBytesIntoFd(&cmd, 1);
  writeBytesIntoFd(&mem_desc, sizeof(mem_desc));
}

void Backend::writeMemory(void *memory_ptr, void *memory_addr,
                          size_t memory_size) {
  assert(comm_fd_ >= 0);

  MemDescMsg mem_desc;
  char cmd = kIPIWriteMem;
  mem_desc.addr = reinterpret_cast<long long>(memory_addr);
  mem_desc.size = memory_size;

  writeBytesIntoFd(&cmd, 1);
  writeBytesIntoFd(&mem_desc, sizeof(mem_desc));
  writeBytesIntoFd(memory_ptr, memory_size);
}

void Backend::readMemory(void *memory_ptr, void *memory_addr,
                         size_t memory_size) {
  assert(comm_fd_ >= 0);

  MemDescMsg mem_desc;
  char cmd = kIPIReadMem;
  mem_desc.addr = reinterpret_cast<long long>(memory_addr);
  mem_desc.size = memory_size;

  writeBytesIntoFd(&cmd, 1);
  writeBytesIntoFd(&mem_desc, sizeof(mem_desc));
  readBytesFromFd(memory_ptr, memory_size);
}

void Backend::launch(uint64_t launch_pc, const et_runtime::device::layer_dynamic_info &params) {

  executeCodeAndWait(launch_pc, params);
}

void Backend::executeCodeAndWait(uint64_t launch_pc, const et_runtime::device::layer_dynamic_info &params) {

  fprintf(stderr,
          "  tensor_a = 0x%" PRIx64 "\n"
          "  tensor_b = 0x%" PRIx64 "\n"
          "  tensor_c = 0x%" PRIx64 "\n"
          "  tensor_d = 0x%" PRIx64 "\n"
          "  tensor_e = 0x%" PRIx64 "\n"
          "  tensor_f = 0x%" PRIx64 "\n"
          "  tensor_g = 0x%" PRIx64 "\n"
          "  tensor_h = 0x%" PRIx64 "\n"
          "  pc/id    = 0x%" PRIx64 "\n",
          params.tensor_a, params.tensor_b, params.tensor_c,
          params.tensor_d, params.tensor_e, params.tensor_f,
          params.tensor_g, params.tensor_h, params.kernel_id);

  ExecuteDescMsg execute_desc;
  char cmd = kIPIExecute;
  execute_desc.launch_pc = launch_pc;
  execute_desc.params = params;

  writeBytesIntoFd(&cmd, 1);
  writeBytesIntoFd(&execute_desc, sizeof(execute_desc));

  cmd = kIPISync;
  char resp;
  writeBytesIntoFd(&cmd, 1);
  readBytesFromFd(&resp, 1);
}

void Backend::shutdown(const char *work_dir) {
  if (comm_socket_ != -1) {
    int status;
    char cmd = kIPIShutdown;

    writeBytesIntoFd(&cmd, 1);

    waitpid(emu_pid_, &status, 0);

    close(comm_socket_);
    close(comm_fd_);

    unlink((std::string(work_dir) + "/" + getCommunicationFile()).c_str());
  }
}
