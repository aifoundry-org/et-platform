import sys
from builtins import bytes
import os

sys.path.append(".")

# Import test framework
from tf.tf_sp_trace_utils import TfSpTraceUtils
from tf.tf_test_helpers import TfTestHelpers

SP_TRACE_LOG_FILE = 'TfSysEmuSpTrace.log'


def test_env_initialize():
    print('Setup environment')
    global test_helpers
    test_helpers = TfTestHelpers('sim')
    global tf_spec
    tf_spec = test_helpers.tf_spec
    global dut_fifo_iface
    dut_fifo_iface = test_helpers.dut_iface

    # In order to get the enough SP trace buffer, lets first configure it and then set the
    # require intercept level

    # Initialize trace helper
    global sp_trace_helper
    sp_trace_helper = TfSpTraceUtils(tf_spec)
    os.system("rm -rf " + SP_TRACE_LOG_FILE)
    print('Run control the SP trace...')
    response = dut_fifo_iface.execute_test(sp_trace_helper.get_control_cmd())
    assert response["payload_size"] == 0
    print('Configure the SP trace...')
    response = dut_fifo_iface.execute_test(sp_trace_helper.get_config_cmd())
    assert response["payload_size"] == 0

    # Set device TF interception point to TF_BL2_ENTRY_FOR_DM
    response = test_helpers.set_intercept_point('TF_BL2_ENTRY_FOR_DM')
    assert response["current_intercept"] == tf_spec.data["sp_tf_interception_points"]["TF_BL2_ENTRY_FOR_DM"]


def test_sp_get_buffer_with_base():
    print('Getting SP trace buffer with trace base address...')

    # find trace base and offset address
    response = dut_fifo_iface.execute_test(sp_trace_helper.get_trace_info_cmd())
    base = response["base"]
    offset = response["offset"]
    size = response["size"]
    print("SP Trace Base Addr: " + hex(base))
    print("SP Trace Offset: " + hex(offset))
    print("Sp Trace Size: " + hex(size))

    # Get trace for given offset and base address
    response = dut_fifo_iface.execute_test(sp_trace_helper.get_trace_buffer_for_offset_cmd(base, offset))
    assert response["bytes_read"] == offset

    # process trace buffer
    print('Printing trace buffer...\n')
    buffer = sp_trace_helper.parse_trace_buffer(response)
    for s in buffer:
        print(s)


def print_trace_buffer(command_name='', to_file=True):
    response = dut_fifo_iface.execute_test(sp_trace_helper.get_trace_buffer_cmd())
    # parse trace buffer
    buffer = sp_trace_helper.parse_trace_buffer(response)

    if to_file:
        with open(SP_TRACE_LOG_FILE, 'a') as f:
            print("Printing trace buffer '" + command_name + "' ...\n", file=f)
            for s in buffer:
                print(s, file=f)
    else:
        print("Printing trace buffer '" + command_name + "' ...\n")
        for s in buffer:
            print(s)


def test_get_sp_trace():
    print('Getting SP trace buffer...')
    print_trace_buffer("TF_RSP_SP_TRACE_GET_BUFFER")

def test_tf_bl2_entry_for_dm():
    # Set device TF interception point to TF_BL2_ENTRY_FOR_DM
    response = test_helpers.set_intercept_point('TF_BL2_ENTRY_FOR_DM')
    assert response["current_intercept"] == tf_spec.data["sp_tf_interception_points"]["TF_BL2_ENTRY_FOR_DM"]


# Helper for asset tracking tests
def check_asset_tracking(cmd, ret_field, ret_expected):
    command = tf_spec.command(cmd, "SP")
    response = dut_fifo_iface.execute_test(command)
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


# TODO: This is only supported for ZEBU. Port this for ZEBU
# check_asset_tracking("TF_CMD_AT_CHIP_REVISION", "chip_rev", "")

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


def test_echo_to_sp():
    print('Testing echo to SP..')
    # Initialize command params
    expected_payload = 0xDEADBEEF
    # Instantiate test command
    command = tf_spec.command("TF_CMD_ECHO_TO_SP", "SP", expected_payload)
    # Issue test command
    response = dut_fifo_iface.execute_test(command)
    # validate response using relevant assertions
    assert response["cmd_payload"] == expected_payload


def test_sp_fw_ver():
    print('SP FW version ..')
    command = tf_spec.command("TF_CMD_SP_FW_VERSION", "SP")
    response = dut_fifo_iface.execute_test(command)
    assert response["major"] == 0x01
    assert response["minor"] == 0x02
    assert response["revision"] == 0x03


# Validate SRAM memory access
def test_move_data_between_host_and_device():
    print('Move data to device ..')
    # Address of SRAM HI IO region
    device_addr = 0x20020000
    test_data = bytes(b'123456789')
    test_data_len = len(test_data)
    command = tf_spec.command("TF_CMD_MOVE_DATA_TO_DEVICE", "SP", device_addr, test_data_len, test_data)
    response = dut_fifo_iface.execute_test(command)
    assert response["bytes_written"] == test_data_len

    # Read back and validate test data
    command = tf_spec.command("TF_CMD_MOVE_DATA_TO_HOST", "SP", device_addr, test_data_len)
    response = dut_fifo_iface.execute_test(command)
    assert response["bytes_read"] == test_data_len
    assert test_data == response["data_ptr"].to_bytes(response["bytes_read"], 'little')


def test_env_finalize():
    test_helpers.teardown_iface()
