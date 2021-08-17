import pytest
import time, sys
from builtins import bytes
import os

sys.path.append(".")

# Import test framework
from tf.test_env import TfEnv
from tf.test_env import TF_SPEC
from tf.tf_specification import TfSpecification
from tf.tf_target_fifo import TargetFifo
from tf.tf_sp_trace_utils import TfSpTraceUtils

SP_TRACE_LOG_FILE = 'TfSysEmuSpTrace.log'

def test_env_initialize():
    print('Setup environment')
    global env
    env = TfEnv("sim")
    global tf_spec
    tf_spec = TfSpecification(TF_SPEC)
    global sp_trace_helper
    sp_trace_helper = TfSpTraceUtils(tf_spec)
    os.system("rm -rf " + SP_TRACE_LOG_FILE)
    global dut_fifo_iface
    dut_fifo_iface = TargetFifo("run/sp_uart1_tx", "run/sp_uart1_rx", tf_spec)
    dut_fifo_iface.open()
    #Set device TF interception point to TF_BL2_ENTRY_FOR_SP_MM
    print('TF interception point..')
    tf_interception_points = tf_spec.data["sp_tf_interception_points"]
    #Initialize command params
    tf_device_rt_intercept = tf_interception_points["TF_BL2_ENTRY_FOR_SP_MM"]
    #Instantiate test command
    command = tf_spec.command("TF_CMD_SET_INTERCEPT", "SP", tf_device_rt_intercept)
    #Issue test command
    response = dut_fifo_iface.execute_test(command)
    #validate response using relevant assertions
    assert response["current_intercept"] == tf_device_rt_intercept
    #tf_spec.view_json()


# Run control SP trace
def test_sp_trace_contol():
    print('Run control sp trace test')
    response = dut_fifo_iface.execute_test(sp_trace_helper.get_control_cmd())
    assert response["payload_size"] == 0


# Run configure SP trace
def test_sp_trace_configure():
    print('Configure sp trace test')
    response = dut_fifo_iface.execute_test(sp_trace_helper.get_config_cmd())
    assert response["payload_size"] == 0


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
    assert size == 4096

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
            print("Printing trace buffer '" +  command_name  + "' ...\n", file=f)
            for s in buffer:
                print(s, file=f)
    else:
        print("Printing trace buffer '" +  command_name  + "' ...\n")
        for s in buffer:
            print(s)

def test_get_sp_trace():
    print('Getting SP trace buffer...')
    print_trace_buffer("TF_RSP_SP_TRACE_GET_BUFFER", to_file=False)

#Example test with 1 command arg, and 1 payload arg
def test_echo_to_sp():
    print('Testing echo to SP..')
    #Initialize command params
    expected_payload = 0xDEADBEEF
    #Instantiate test command
    command = tf_spec.command("TF_CMD_ECHO_TO_SP", "SP", expected_payload)
    #Issue test command
    response = dut_fifo_iface.execute_test(command)
    print_trace_buffer('TF_CMD_ECHO_TO_SP')
    #validate response using relevant assertions
    assert response["cmd_payload"] == expected_payload

#Example test with no command args, and 3 payload args
def test_sp_fw_ver():
    print('SP FW version ..')
    command = tf_spec.command("TF_CMD_SP_FW_VERSION", "SP")
    response = dut_fifo_iface.execute_test(command)
    print_trace_buffer('TF_CMD_SP_FW_VERSION')
    assert response["major"] == 0x01
    assert response["minor"] == 0x02
    assert response["revision"] == 0x03

#Example test to move data to device
def test_move_data_between_host_and_device():
    print('Move data to device ..')
    device_addr = 0x0000000010000000
    test_data = bytes(b'123456789')
    test_data_len = len(test_data)
    command = tf_spec.command("TF_CMD_MOVE_DATA_TO_DEVICE", "SP", device_addr, test_data_len, test_data)
    response = dut_fifo_iface.execute_test(command)
    print_trace_buffer('TF_CMD_MOVE_DATA_TO_DEVICE')
    assert response["bytes_written"] == test_data_len
    command = tf_spec.command("TF_CMD_MOVE_DATA_TO_HOST", "SP", device_addr, test_data_len)
    response = dut_fifo_iface.execute_test(command)
    print_trace_buffer('TF_CMD_MOVE_DATA_TO_HOST')
    assert response["bytes_read"] == test_data_len

def test_env_finalize():
    dut_fifo_iface.close()
    env.finalize("sim")
    print('Tear down environment')
