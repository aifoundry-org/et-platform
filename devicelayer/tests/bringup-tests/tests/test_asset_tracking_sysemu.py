import pytest
import time, sys
from builtins import bytes

sys.path.append(".")

# Import test framework
from tf.test_env import TfEnv
from tf.tf_specification import TfSpecification
from tf.tf_target_fifo import TargetFifo

def test_env_initialize():
    print('Setup environment')
    global env
    env = TfEnv("sim")
    global tf_spec
    tf_spec = TfSpecification("tf/tf_specification.json")
    global dut_fifo_iface
    dut_fifo_iface = TargetFifo("run/sp_uart1_tx", "run/sp_uart1_rx", tf_spec)
    dut_fifo_iface.open()
    #tf_spec.view_json()

# Helper for asset tracking tests
def check_asset_tracking(cmd, ret_field, ret_expected):
    command = tf_spec.command(cmd, "SP")
    print(str(command))
    response = dut_fifo_iface.execute_test(command)
    tf_spec.prettyprint(response)
    ret = response[ret_field].to_bytes(response["payload_size"], 'little')
    ret = str(ret, 'utf-8').rstrip('\x00')
    assert ret == ret_expected

def test_at_manufacturer_name():
    print('Asset Tracking Manufacturer Name ..')
    check_asset_tracking("TF_CMD_AT_MANUFACTURER_NAME", "mfr_name", "Esperan")

def test_at_part_number():
    print('Asset Tracking Part Number ..')
    check_asset_tracking("TF_CMD_AT_PART_NUMBER", "part_num", "ETPART1")

def test_at_serial_number():
    print('Asset Tracking Serial Number ..')
    check_asset_tracking("TF_CMD_AT_SERIAL_NUMBER", "ser_num", "ETSER_1")

def test_at_chip_revision():
    print('Asset Tracking Chip Revision ..')
    # TODO: fix expected value when the test is ported over to zebu
    #       probably true for most of the AT tests
    check_asset_tracking("TF_CMD_AT_CHIP_REVISION", "chip_rev", "")

def test_at_pcie_max_speed():
    print('Asset Tracking PCIe Max Speed ..')
    check_asset_tracking("TF_CMD_AT_PCIE_MAX_SPEED", "pcie_max_speed", "8")

def test_at_module_revision():
    print('Asset Tracking Module Revision ..')
    check_asset_tracking("TF_CMD_AT_MODULE_REVISION", "module_rev", "1")

def test_at_form_factor():
    print('Asset Tracking Form Factor ..')
    check_asset_tracking("TF_CMD_AT_FORM_FACTOR", "form_factor", "Dual_M2")

def test_at_memory_details():
    print('Asset Tracking Memory Details ..')
    check_asset_tracking("TF_CMD_AT_MEMORY_DETAILS", "mem_details", "Unknown")

def test_at_memory_size_mb():
    print('Asset Tracking Memory Size MB ..')
    check_asset_tracking("TF_CMD_AT_MEMORY_SIZE_MB", "mem_size_mb", "16384")

def test_at_memory_type():
    print('Asset Tracking Memory Type ..')
    check_asset_tracking("TF_CMD_AT_MEMORY_TYPE", "mem_type", "LPDDR4X")

def test_env_finalize():
    dut_fifo_iface.close()
    env.finalize("sim")
    print('Tear down environment')
