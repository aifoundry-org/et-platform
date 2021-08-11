import pytest
import time, sys
from builtins import bytes

sys.path.append(".")

# Import test framework
from tf.test_env import TfEnv
from tf.test_env import TF_SPEC
from tf.tf_specification import TfSpecification
from tf.tf_target_fifo import TargetFifo

def test_env_initialize():
    print('Setup environment')
    global env
    env = TfEnv("sim")
    global tf_spec
    tf_spec = TfSpecification(TF_SPEC)
    global dut_fifo_iface
    dut_fifo_iface = TargetFifo("run/sp_uart1_tx", "run/sp_uart1_rx", tf_spec)
    dut_fifo_iface.open()
    #tf_spec.view_json()

#Example test with 1 command arg, and 1 payload arg
def test_echo_to_sp():
    print('Testing echo to SP..')
    #Initialize command params
    expected_payload = 0xDEADBEEF
    #Instantiate test command
    command = tf_spec.command("TF_CMD_ECHO_TO_SP", "SP", expected_payload)
    #Issue test command
    response = dut_fifo_iface.execute_test(command)
    #validate response using relevant assertions
    assert response["cmd_payload"] == expected_payload

#Example test with no command args, and 3 payload args
def test_sp_fw_ver():
    print('SP FW version ..')
    command = tf_spec.command("TF_CMD_SP_FW_VERSION", "SP")
    response = dut_fifo_iface.execute_test(command)
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
    assert response["bytes_written"] == test_data_len
    command = tf_spec.command("TF_CMD_MOVE_DATA_TO_HOST", "SP", device_addr, test_data_len)
    response = dut_fifo_iface.execute_test(command)
    assert response["bytes_read"] == test_data_len

def test_env_finalize():
    dut_fifo_iface.close()
    env.finalize("sim")
    print('Tear down environment')
