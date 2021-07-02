//******************************************************************************
// Copyright (C) 2021 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "test_idevice_mgmt_api.h"
#include <gmock/gmock.h>

class TestIDeviceMgmtApiPcieSysEmu : public TestIDeviceMngtAPI {
protected:

  void SetUp() override {

    tag_id_ = 0x8000;
    fListenerTimeout_ = std::chrono::seconds(300);

    // Launch PCIE through IDevice Abstraction
    devLayer_ = dev::IDeviceLayer::createPcieDeviceLayer(false, true);
  }
};

TEST_F(TestIDeviceMgmtApiPcieSysEmu, GET_MODULE_MANUFACTURE_NAME) {
  // Manufacture name function, PCIE fixture
  getModuleManufactureName();
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
