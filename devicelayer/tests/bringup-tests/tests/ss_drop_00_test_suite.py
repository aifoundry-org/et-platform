import sys

sys.path.append(".")

# Import test framework
from tf.tf_test_helpers import TfTestHelpers


def test_env_initialize():
    print('Setup environment')
    global test_helpers
    test_helpers = TfTestHelpers('sim')
    global tf_spec
    tf_spec = test_helpers.tf_spec

    # Set device TF interception point to TF_BL2_ENTRY_FOR_HW
    response = test_helpers.set_intercept_point('TF_BL2_ENTRY_FOR_HW')
    assert response["current_intercept"] == tf_spec.data["sp_tf_interception_points"]["TF_BL2_ENTRY_FOR_HW"]


def test_env_finalize():
    test_helpers.teardown_iface()