import pytest
import time, sys
import os
from builtins import bytes

sys.path.append(".")

# Import test framework
from tf.test_env import TfEnv
from tf.test_env import TF_SPEC
from tf.tf_specification import TfSpecification
from tf.tf_target_fifo import TargetFifo
from tf.tf_test_helpers import TfTestHelpers

def test_env_initialize():
    print('Setup environment for testing MM commands over fifo interface to SysEMU')
    global env
    env = TfEnv("sim")
    global tf_spec
    tf_spec = TfSpecification(TF_SPEC)
    global dut_fifo_iface
    dut_fifo_iface = TargetFifo("run/sp_uart1_tx", "run/sp_uart1_rx", tf_spec)
    dut_fifo_iface.open()
    global TestHelpers
    TestHelpers = TfTestHelpers()
    time.sleep(1) #give time for MM to boot up
    #Set device TF interception point to TF_BL2_ENTRY_FOR_SP_MM
    print('TF interception point..')
    tf_interception_points = tf_spec.data["sp_tf_interception_points"]
    #Initialize command params
    tf_device_rt_intercept = tf_interception_points["TF_BL2_ENTRY_FOR_SP_MM"]
    #tf_device_rt_intercept = 5
    #Instantiate test command
    command = tf_spec.command("TF_CMD_SET_INTERCEPT", "SP", tf_device_rt_intercept)
    #Issue test command
    response = dut_fifo_iface.execute_test(command)
    #validate response using relevant assertions
    assert response["current_intercept"] == tf_device_rt_intercept
    #tf_spec.view_json()

#Validate mm heart beat to assess if MM is alive
def test_mm_heart_beat():
    print('Testing for MM heart beat..')
    cmd = tf_spec.command("TF_CMD_GET_MM_HEARTBEAT", "SP")
    response = dut_fifo_iface.execute_test(cmd)
    if(response["heartbeat_count"] == 0xFFFFFFFFFFFFFFFF):
        print("SP BL2 lacks heart beat functionality at this time")
    assert response["heartbeat_count"] != 0

#Validate echo to mm to determine if MM is up to support echo response, and validation SP to MM bi-directional comms
def test_echo_to_mm():
    print('Testing echo to MM..')
    expected_payload = 0xDEADBEEF
    mm_cmd = tf_spec.command("TF_CMD_MM_ECHO", "MM", expected_payload)
    mm_cmd_len = len(mm_cmd)
    sp_cmd = tf_spec.command("TF_CMD_MM_CMD_SHELL", "SP", mm_cmd_len, mm_cmd)
    response = dut_fifo_iface.execute_test(sp_cmd)
    assert response["device_cmd_start_ts"] != 0

#Validate expected MM firmware version
def test_fw_ver_to_mm():
    print('Testing MM FW version..')
    firmware_type = 0 #Master Minion FW
    mm_cmd = tf_spec.command("TF_CMD_MM_FW_VERSION", "MM", firmware_type)
    mm_cmd_len = len(mm_cmd)
    sp_cmd = tf_spec.command("TF_CMD_MM_CMD_SHELL", "SP", mm_cmd_len, mm_cmd)
    response = dut_fifo_iface.execute_test(sp_cmd)
    assert response["type"] == firmware_type
    assert response["major"] == 0x0
    assert response["minor"] == 0x0
    assert response["patch"] == 0x0

#Validate ability to launch an empty kernel on MM
def test_kernel_launch_to_mm():
    print('Testing MM kernel launch..')

    ###############
    # Find kernel #
    ###############
    kernel_name = 'empty.elf'
    kernel_path = TestHelpers.kernel_get_path(kernel_name)

    #################
    # Read ELF Info #
    #################
    kernel_data, kernel_segment_offset = TestHelpers.kernel_read_elf_data(kernel_path)

    #############################
    # Load ELF to device memory #
    #############################
    # kernelfile is the file which has been seeked to the loadable section to device. Copy the data
    # from this offset (size: filesize) to device address
    kernel_device_load_addr = 0x0000008101000000 + kernel_segment_offset

    print('loading kernel data..')
    device_addr = kernel_device_load_addr
    test_data = kernel_data
    test_data_len = len(test_data)
    command = tf_spec.command("TF_CMD_MOVE_DATA_TO_DEVICE", "SP", device_addr, test_data_len, test_data)
    response = dut_fifo_iface.execute_test(command)
    assert response["bytes_written"] == test_data_len

    ###################################
    # Construct kernel launch command #
    ###################################
    code_start_address = kernel_device_load_addr
    pointer_to_args = 0 #No args
    exception_buffer = 0 #No exception buffer
    shire_mask = 0x1FFFFFFFF
    kernel_trace_buffer = 0 #No tracing
    argument_payload = 0
    mm_cmd = tf_spec.command("TF_CMD_MM_KERNEL_LAUNCH", "MM", code_start_address, pointer_to_args, exception_buffer, shire_mask, kernel_trace_buffer, argument_payload)
    mm_cmd_len = len(mm_cmd)
    sp_cmd = tf_spec.command("TF_CMD_MM_CMD_SHELL", "SP", mm_cmd_len, mm_cmd)
    print("Kernel launch command to DUT")
    response = dut_fifo_iface.execute_test(sp_cmd)
    print("DUT response from kernel launch")
    tf_spec.prettyprint(response)
    assert response["status"] == 0

def test_env_finalize():
    dut_fifo_iface.close()
    env.finalize("sim")
    print('Tear down environment')