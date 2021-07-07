import pytest
import time, sys
import os
from elftools.elf.elffile import ELFFile
from elftools.elf.segments import Segment
from builtins import bytes

sys.path.append(".")

# Import test framework
from tf.test_env import TfEnv
from tf.tf_specification import TfSpecification
from tf.tf_target_fifo import TargetFifo

def test_env_initialize():
    print('Setup environment for testing MM commands over fifo interface to SysEMU')
    global env
    env = TfEnv("sim")
    global tf_spec
    tf_spec = TfSpecification("tf/tf_specification.json")
    global dut_fifo_iface
    dut_fifo_iface = TargetFifo("run/sp_uart1_tx", "run/sp_uart1_rx", tf_spec)
    dut_fifo_iface.open()
    time.sleep(1) #give time for MM to boot up

#Example test with 1 command arg, and 1 payload arg
def test_echo_to_mm():
    print('Testing echo to MM..')
    expected_payload = 0xDEADBEEF
    mm_cmd = tf_spec.command("TF_CMD_MM_ECHO", "MM", expected_payload)
    mm_cmd_len = len(mm_cmd)
    sp_cmd = tf_spec.command("TF_CMD_MM_COMMAND_SHELL", "SP", mm_cmd_len, mm_cmd)
    print(str(sp_cmd))
    print(len(sp_cmd))
    response = dut_fifo_iface.execute_test(sp_cmd)
    tf_spec.prettyprint(response)
    #verify echo payload?

#Example test with no command args, and 3 payload args
def test_fw_ver_to_mm():
    print('Testing MM FW version..')
    firmware_type = 0 #Master Minion FW
    mm_cmd = tf_spec.command("TF_CMD_MM_FW_VERSION", "MM", firmware_type)
    mm_cmd_len = len(mm_cmd)
    sp_cmd = tf_spec.command("TF_CMD_MM_COMMAND_SHELL", "SP", mm_cmd_len, mm_cmd)
    print(str(sp_cmd))
    print(len(sp_cmd))
    response = dut_fifo_iface.execute_test(sp_cmd)
    tf_spec.prettyprint(response)
    #verify fw versions?

def test_kernel_launch_to_mm():
    print('Testing MM kernel launch..')

    ###############
    # Find kernel #
    ###############
    kernels_root_dir = os.path.abspath(__file__ + "/../../../../../../") + str('/device-software/')
    print('Kernels root directory:' + kernels_root_dir)
    #find the kernel with the specified name
    kernel_name = 'empty.elf'
    kernel_path = ''
    for root, dirs, files in os.walk(kernels_root_dir):
        if kernel_name in files:
            kernel_path = os.path.join(root, kernel_name)
    print(kernel_name + ' path: ' + kernel_path)

    #################
    # Read ELF Info #
    #################
    kernel_data_offset = 0
    kernel_segment_offset = 0
    kernelfile = open(kernel_path, 'rb')
    elffile = ELFFile(kernelfile)
    elfentry = elffile.header.e_entry
    print(kernel_name + " information:")
    print(f"ELF entry address:{hex(elfentry)}")
    for segment in elffile.iter_segments():
        seg_head = segment.header
        if seg_head.p_type == 'PT_LOAD':
            print(f"Segment Type: {seg_head.p_type}")
            print(f"Offset: {hex(seg_head.p_offset)}")
            print(f"Size in file:{hex(seg_head.p_filesz)}")
            print(f"Size in memory:{hex(seg_head.p_memsz)}")
            print(f"Virtual address:{hex(seg_head.p_vaddr)}")
            print(f"Physical address:{hex(seg_head.p_paddr)}")
            kernel_data_offset = seg_head.p_offset
            kernel_segment_offset = seg_head.p_vaddr - elfentry
    # seek the file to the start of kernel PT_LOAD segment
    kernelfile.seek(kernel_data_offset)

    print(f"\nOffset to be added to kernel load address:{hex(kernel_segment_offset)}")

    #############################
    # Load ELF to device memory #
    #############################
    # TODO: kernelfile is the file which has been seeked to the loadable section to device. Copy the data
    # from this offset (size: 0x1000) to device address
    kernel_device_load_addr = 0x0000000010000000 + kernel_segment_offset

    ###################################
    # Construct kernel launch command #
    ###################################
    code_start_address = kernel_device_load_addr
    pointer_to_args = 0
    exception_buffer = 0 #No exception buffer
    shire_mask = 0x1FFFFFFFF
    kernel_trace_buffer = 0 #No tracing
    argument_payload = 0
    mm_cmd = tf_spec.command("TF_CMD_MM_KERNEL_LAUNCH", "MM", code_start_address, pointer_to_args, exception_buffer, shire_mask, kernel_trace_buffer, argument_payload)
    mm_cmd_len = len(mm_cmd)
    sp_cmd = tf_spec.command("TF_CMD_MM_COMMAND_SHELL", "SP", mm_cmd_len, mm_cmd)
    print(str(sp_cmd))
    print(len(sp_cmd))
    response = dut_fifo_iface.execute_test(sp_cmd)
    tf_spec.prettyprint(response)

def test_env_finalize():
    dut_fifo_iface.close()
    env.finalize("sim")
    print('Tear down environment')