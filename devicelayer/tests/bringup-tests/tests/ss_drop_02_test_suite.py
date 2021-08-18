import sys
import time

sys.path.append(".")

# Import test framework
from tf.tf_test_helpers import TfTestHelpers


def test_env_initialize():
    print('Setup environment')
    global test_helpers
    test_helpers = TfTestHelpers('sim')
    global tf_spec
    tf_spec = test_helpers.tf_spec
    global dut_fifo_iface
    dut_fifo_iface = test_helpers.dut_iface

    # Set device TF interception point to TF_BL2_ENTRY_FOR_DM_WITH_PCIE
    response = test_helpers.set_intercept_point('TF_BL2_ENTRY_FOR_DM_WITH_PCIE')
    assert response["current_intercept"] == tf_spec.data["sp_tf_interception_points"]["TF_BL2_ENTRY_FOR_DM_WITH_PCIE"]
    # wait for pcie link up
    time.sleep(2)


def test_pcie_init_status():
    command = tf_spec.command("TF_CMD_GET_PCIE_INIT_STATUS", "SP")
    response = dut_fifo_iface.execute_test(command)
    assert response["status"] == 0


def test_env_finalize():
    test_helpers.teardown_iface()