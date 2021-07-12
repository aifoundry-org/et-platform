import os
from elftools.elf.elffile import ELFFile

class TfTestHelpers:
    def __init__(self, *args):
        #initialize JTAG target specific instance
        print("Initializing TF tests helper object...")
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