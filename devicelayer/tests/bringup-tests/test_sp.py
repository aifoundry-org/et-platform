import pytest
import time, sys 

sys.path.append(".")

# Import test framework 
from tf_target import *
from tf_specification import *

#@pytest.mark.run(order=1)
def test_env_init(env_setup):
    print('test initialization')
    # Instantiate a global dut_fifo_iface handle
    global dut_fifo_iface
    dut_fifo_iface = Target("fifo", "sp_uart0_tx", "sp_uart0_rx")
    # Instatiate a global dut_serial_iface handle
    #global dut_serial_iface
    #dut_serial_iface = Target("serial", "/dev/ttyS0", 115200)
    #JTAG
    #global dut_JTAG_iface
    #dut_serial_iface = Target("jtag", ...)
    # Instantiate TF Specification handle
    global tf_spec
    tf_spec = tf_specification("tf_specification.json")
    #Open test interface
    dut_fifo_iface.open()
    # For debug: uncomment line below to pretty print json spec
    #tf_spec.view_json()

#@pytest.mark.run(order=2)
#Example test with 1 command arg, and 1 payload arg
def test_echo_to_sp():
    print('Testing echo to SP..')
    #Initialize command params
    expected_payload = 0xDEADBEEF
    #Instantiate test command
    command = tf_spec.command("TF_CMD_ECHO_TO_SP", expected_payload)
    #print("From host:" + str(command))
    #Issue test command
    raw_response = dut_fifo_iface.execute_test(command)
    #print("From device:" + str(raw_response))
    #Parse test response
    response = tf_spec.response(raw_response)
    tf_spec.prettyprint(response)
    #validate response using relevant assertions
    assert response["cmd_payload"] == expected_payload

#@pytest.mark.run(order=3)
#Example test with 1 command arg, and 1 payload arg
def test_echo_to_mm():
    print('Testing echo to MM..')
    expected_payload = 0xDEADBEEF
    command = tf_spec.command("TF_CMD_ECHO_TO_MM", expected_payload)
    raw_response = dut_fifo_iface.execute_test(command)
    response = tf_spec.response(raw_response)
    tf_spec.prettyprint(response)
    assert response["cmd_payload"] == expected_payload

#@pytest.mark.run(order=4)
#Example test with no command args, and 3 payload args
def test_sp_fw_ver():
    print('SP FW version ..')
    command = tf_spec.command("TF_CMD_SP_FW_VERSION")
    raw_response = dut_fifo_iface.execute_test(command)
    response = tf_spec.response(raw_response)
    tf_spec.prettyprint(response)
    assert response["major"] == 0x01
    assert response["minor"] == 0x02
    assert response["revision"] == 0x03

#@pytest.mark.run(order=5)
#Example test with no command args, and 3 payload args
def test_mm_fw_ver():
    print('MM FW Version ..')
    command = tf_spec.command("TF_CMD_MM_FW_VERSION")
    raw_response = dut_fifo_iface.execute_test(command)
    response = tf_spec.response(raw_response)
    tf_spec.prettyprint(response)
    assert response["major"] == 0x01
    assert response["minor"] == 0x02
    assert response["revision"] == 0x03

#@pytest.mark.run(order=6)
def test_env_finalize():
    print('test finalize')
    dut_fifo_iface.close()
    #teardown system here 
