#include "EsperantoRuntime.h"

#include <cstdlib>
#define STRIP_FLAG_HELP 1
#include <gflags/gflags.h>
//#include <glog/logging.h>
#include <iostream>

DEFINE_bool(verbose, false, "Display program name before message");
DEFINE_string(message, "Hello world!", "Message to print");

int main(int argc, char *argv[]) {
  gflags::SetUsageMessage("ET Runtime SystemTest helper");
  gflags::SetVersionString("0.0.1");
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  if (FLAGS_verbose)
    std::cout << gflags::ProgramInvocationShortName() << ": ";
  std::cout << FLAGS_message << std::endl;
  return 0;
}
