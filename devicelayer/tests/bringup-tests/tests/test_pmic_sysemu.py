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

# PMIC helper
def check_pmic_ranged(cmd, ret_field, ret_min, ret_max):
    command = tf_spec.command(cmd)
    print(str(command))
    response = dut_fifo_iface.execute_test(command)
    tf_spec.prettyprint(response)
    assert (response[ret_field] >= ret_min and response[ret_field] <= ret_max)

# PMIC tests
# Should fail on sysemu as the temperature/power isn't emulated
def test_pmic_module_temperature():
    print('PMIC Module Temperature ..')
    check_pmic_ranged("TF_CMD_PMIC_MODULE_TEMPERATURE", "mod_temperature", 52, 85)

def test_pmic_module_power():
    print('PMIC Module Power ..')
    check_pmic_ranged("TF_CMD_PMIC_MODULE_POWER", "mod_power", 15, 40)

def test_env_finalize():
    dut_fifo_iface.close()
    env.finalize("sim")
    print('Tear down environment')
