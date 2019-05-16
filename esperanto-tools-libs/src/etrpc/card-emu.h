//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_ETRPC_CARD_EMU_H
#define ET_RUNTIME_ETRPC_CARD_EMU_H

// This is converts card-emu in a class that the et-rpc can call
// directly and eliminate 1 degree of indiretion.

#include "backend.h"
#include "et-card-proxy.h"
#include "etrpc/et-rpc.h"

#include <memory>
#include <string>

class CardEmu {
public:
  CardEmu(const std::string &sysemu_socket_path)
      : sysemu_socket_path_(sysemu_socket_path),
        gBackendInterface_(std::make_unique<BackendSysEmu>()) {}

  bool init();
  void shutdown();

  MessageDefineRes defineReq(MessageDefineReq *req);
  std::unique_ptr<MessageReadRes> readMemReq(MessageReadReq *req);
  MessageWriteRes writeMemReq(MessageWriteReq *req);
  MessageBootRes bootReq(MessageBootReq *req);
  MessageLaunchRes launchReq(MessageLaunchReq *req);

private:
  std::string sysemu_socket_path_;

  std::unique_ptr<BackendSysEmu> gBackendInterface_;
};

#endif // ET_RUNTIME_ETRPC_CARD_EMU_H
