/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "MailboxReader.h"
#include "NewCore/RuntimeImp.h"
#include "esperanto/device-api/device_api_message_types.h"
#include "esperanto/device-api/device_api_spec_non_privileged.h"
#include "utils.h"
#include <array>
#include <thread>
using namespace rt;

MailboxReader::MailboxReader(ITarget* target, KernelParametersCache* kernelParametersCache, EventManager* eventManager)
  : run_(true)
  , target_(target)
  , eventManager_(eventManager)
  , kernelParametersCache_(kernelParametersCache) {

  reader_ = std::thread([this]() {
    std::array<std::byte, 1 << 16> buffer;
    while (run_) {
      auto readResult = target_->readMailbox(buffer.data());
      if (readResult) {
        // grab header
        // notify events (dispatch)
        // free parameters buffer associated to kernel if any (and if it was a kernel)
        auto response = reinterpret_cast<response_header_t*>(buffer.data());
        auto msgid = response->message_id;
        RT_DLOG(INFO) << "MsgId:" << msgid;

        if (MBOX_DEVAPI_NON_PRIVILEGED_MID_NONE < msgid && msgid < MBOX_DEVAPI_NON_PRIVILEGED_MID_LAST) {
          auto eventId = EventId(response->command_info.command_id);
          eventManager_->dispatch(eventId);
          kernelParametersCache_->releaseBuffer(eventId);
        }
      }
    }
  });
}

MailboxReader::~MailboxReader() {
  run_ = false;
  reader_.join();
}

