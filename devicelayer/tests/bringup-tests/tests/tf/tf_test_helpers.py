import os
from elftools.elf.elffile import ELFFile

from tf.test_env import TfEnv
from tf.test_env import TF_SPEC
from tf.tf_specification import TfSpecification
from tf.tf_target_fifo import TargetFifo

class TfTestHelpers:
    def __init__(self, iface):
        self.iface_name = iface
        if self.iface_name == "sim":
            print("Initializing TF tests helper object for SysEmu...")
            os.system("cd ..")
            self.env = TfEnv(self.iface_name)
            self.tf_spec = TfSpecification(TF_SPEC)
            self.dut_iface = TargetFifo("run/sp_uart1_tx", "run/sp_uart1_rx", self.tf_spec)
            self.dut_iface.open()

    def set_intercept_point(self, intercept):
        print("Setting TF interception '" + intercept + "' point ..")
        tf_interception_points = self.tf_spec.data["sp_tf_interception_points"]
        # Initialize command params
        tf_device_rt_intercept = tf_interception_points[intercept]
        command = self.tf_spec.command("TF_CMD_SET_INTERCEPT", "SP", tf_device_rt_intercept)
        # Issue test command
        response = self.dut_iface.execute_test(command)
        return response

    def teardown_iface(self):
        if self.iface_name == 'sim':
            self.dut_iface.close()
            self.env.finalize(self.iface_name)

        print('Tear down environment')

    def kernel_get_path(self, kernel_name):
        kernels_root_dir = os.path.abspath(__file__ + "/../../../../../../../") + str('/device-software/')
        print('Kernels root directory:' + kernels_root_dir)
        #find the kernel with the specified name
        kernel_path = ''
        for root, dirs, files in os.walk(kernels_root_dir):
            if kernel_name in files:
                kernel_path = os.path.join(root, kernel_name)
        print(kernel_name + ' path: ' + kernel_path)
        return kernel_path
    def kernel_read_elf_data(self, kernel_path):
        kernel_data_offset = 0
        kernel_segment_offset = 0
        kernelfile = open(kernel_path, 'rb')
        elffile = ELFFile(kernelfile)
        elfentry = elffile.header.e_entry
        print("Kernel information:")
        print(f"ELF entry address:{hex(elfentry)}")
        for segment in elffile.iter_segments():
            seg_head = segment.header
            if seg_head.p_type == 'PT_LOAD':
                print(f"Segment Type: {seg_head.p_type}")
                print(f"Offset: {hex(seg_head.p_offset)}")
                print(f"Size in file:{hex(seg_head.p_filesz)}")
                filesize = seg_head.p_filesz
                print(f"Size in memory:{hex(seg_head.p_memsz)}")
                print(f"Virtual address:{hex(seg_head.p_vaddr)}")
                print(f"Physical address:{hex(seg_head.p_paddr)}")
                kernel_data_offset = seg_head.p_offset
                kernel_segment_offset = seg_head.p_vaddr - elfentry
        # seek the file to the start of kernel PT_LOAD segment
        kernelfile.seek(kernel_data_offset)
        kernel_data = kernelfile.read(filesize)
        kernelfile.close()
        print(f"\nOffset to be added to kernel load address:{hex(kernel_segment_offset)}")

        return kernel_data, kernel_segment_offset