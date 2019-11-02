import time
import random
import sys
import argparse
import os

# TBD: Need to pass number of tensor loads / fmas in case we have more than one.
# The the variables will have a suffix _id at the end

# Num is the number of tensor load.
# TBD -- Random seed before all generate functions, not just in the first function
# The id has nothing to do wit the ID bit used by SW to add tensor waits.
# Need to find out if and how we may use it
def generate_tensorloads(shires, tl_id, target_dir, output_type, num_iter, total_iter, max_num_cooploads_in_shire, min_coop_id, use_tenb = False):
    
    assert(tl_id == 0 or tl_id == 1)

    assert(not(total_iter > 1 and output_type == "switch"))

    # Overwrite possible existing file on the 1st iteration
    if num_iter == 0:
        tl_configs_hf = open(target_dir+"/tl"+str(tl_id)+"_configs.h","w")        
        if output_type == "switch":
            def_line = "switch(minion_id) {\n"
        elif output_type =="array":
            def_line = "static uint64_t tl"+str(tl_id)+"_configs["+str(total_iter*shires*32*10)+"] = {\n"
            def_line = def_line + "// COOP_CSR  IS_COOP  TMASK  TRANS  SCP_START  TENB  ADDR_OFFSET  LINE_OFFSET  LINES  STRIDE\n"
        tl_configs_hf.write(def_line)
    else:
        tl_configs_hf = open(target_dir+"/tl"+str(tl_id)+"_configs.h","a+")         
   
    full_tl_tenb_list = []
    full_tl_num_lines_list = []

    for sh in range(shires):
        
        # Set the load parameters assuming there are no coops
        # Todo: If tmask_list = 1 you need to set up the tensor mask register
        tl_tmask_list = [random.randint(0,1) for i in range(32)]
        tl_coop_list = [0] *32

        tl_code_notrans = 0
        tl_code_li8 = 1
        tl_code_li16 = 2
        tl_code_lt8 = 5
        tl_code_lt16 = 6
        tl_code_lt32 = 7
        tl_code_map = { 0: tl_code_notrans,
                        1: tl_code_li8,
                        2: tl_code_li16,
                        3: tl_code_lt8,
                        4: tl_code_lt16,
                        5: tl_code_lt32 }                
        
        tl_scp_start_line_list =  [random.randint(0,63) for i in range(32)]

        # Doing evertyhing on TenB may not always make sense
        # The test has to decide this, so set it to 0 here...
        if use_tenb:
            tl_tenb_list = [1] * 32
        else:
            tl_tenb_list = [0] * 32

        # Add option for optional tenb: random.randint(0,1) for i in range(32)]

        tl_code_list = [tl_code_map[random.randint(0,5)] for i in range(32)]
        for i in range(32):
            if tl_tenb_list[i] == 1:
                tl_code_list[i] = 0

        # The address is the offset from some start address specified in the test
        # The limit is specified by #shires, there is 1KB per minion, so 32KB (0x8000 per shire)
        tl_addr_list = [random.randint(0, 0x8000 * shires - 1) & 0xFFFFFFFFFFC0 for i in range(32)]
        
        tl_offset_list = [0] * 32

        tl_num_lines_list = [random.randint(11,15) for i in range(32)]

        # Stride here is speficied in lines
        tl_stride_list =  [(random.randint(1,1024) << 6) for i in range(32)]

        tl_coop_csr_list = [0] * 32

        num_cooploads_in_shire = 0
        
        tl_coop_programmed = [0] * 32

        # Do the coops first -- they share parameters
        #print ("Coops:", num_cooploads_in_shire)
        for coop_ld in range(max_num_cooploads_in_shire):
            
            neigh_mask = random.randint(1,15)
            min_mask = random.randint(1,255)

            # Fix the bitfield mask            
            bitfield_min_mask = [int(x) for x in list('{0:0>8b}'.format(min_mask))]
            bitfield_min_mask.reverse()
            bitfield_shire_mask = []
            #print (neigh_mask,min_mask, bitfield_min_mask)
            for i in range(4):
                if (neigh_mask & (1 << i)):                    
                    bitfield_shire_mask = bitfield_shire_mask + bitfield_min_mask
                else:
                    bitfield_shire_mask = bitfield_shire_mask + ([0] * 8)
            
            coop_possible = True
            first_min = -1
            #print(bitfield_shire_mask)
            for m in range(32):
                if tl_coop_programmed[m] == 0 and bitfield_shire_mask[m] == 1 and first_min == -1:
                    first_min = m
                if tl_coop_programmed[m] == 1 and bitfield_shire_mask[m] == 1:
                    coop_possible = False

            if not coop_possible:                 
                continue
            else:
                num_cooploads_in_shire = num_cooploads_in_shire + 1
            
            #print (first_min)
            # Now go through the minions participating and generate the TL fields.
            # Set also the Tensor Coop CSR
            tl_coop_programmed[first_min] = 1
            tl_coop_list[first_min] = 1
            tl_coop_csr_list[first_min] = tl_coop_csr_list[first_min] | (neigh_mask << 16) + (min_mask << 8) | ((min_coop_id[sh] + num_cooploads_in_shire - 1) % 16)
            for m in range(first_min+1,32):                
                if bitfield_shire_mask[m] == 1:
                    tl_coop_programmed[m] = 1
                    tl_coop_csr_list[m] = tl_coop_csr_list[first_min]
                    tl_tmask_list[m] = tl_tmask_list[first_min]
                    tl_coop_list[m] = 1
                    tl_code_list[m] = tl_code_list[first_min]
                    tl_scp_start_line_list[m] = tl_scp_start_line_list[first_min]
                    tl_tenb_list[m] = tl_tenb_list[first_min]
                    tl_addr_list[m] = tl_addr_list[first_min]
                    if tl_code_list[m] == tl_code_li8 or tl_code_list[m] == tl_code_lt8:
                        tl_offset_list[m] = random.randint(0,3)
                    elif tl_code_list[m] == tl_code_li16 or tl_code_list[m] == tl_code_lt16:
                         tl_offset_list[m] = random.randint(0,1) * 2
                    tl_num_lines_list[m] = tl_num_lines_list[first_min]
                    tl_stride_list[m] = tl_stride_list[first_min]                    
                    
        # All the TL parameters have now been generated.
        # The test may only change the TenB depending what it wants to do
        #tl_mappings_hf = open("tl_mappings.h","w")
        
        #tl_mappings_hf.write(def_line)
        #num_minions = 32 * shires
        if (output_type == "switch"):
            for i in range(0,32):        
                def_line = " case " + str(sh*32+i) + ":\n   TL" + str(tl_id) + "_COOP_CSR = " + str(hex(tl_coop_csr_list[i])) + ";\n"
                def_line = def_line + "   TL" + str(tl_id) + "_IS_COOP = " + str(tl_coop_list[i]) + ";\n"
                def_line = def_line + "   TL" + str(tl_id) + "_TMASK = " + str(tl_tmask_list[i]) + ";\n"
                def_line = def_line + "   TL" + str(tl_id) + "_CODE = " + str(tl_code_list[i]) + ";\n"
                def_line = def_line + "   TL" + str(tl_id) + "_SCP_START_LINE = " + str(tl_scp_start_line_list[i]) + ";\n"
                def_line = def_line + "   TL" + str(tl_id) + "_TENB = " + str(tl_tenb_list[i]) + ";\n"
                def_line = def_line + "   TL" + str(tl_id) + "_ADDR = " + str(hex(tl_addr_list[i])) + ";\n"
                def_line = def_line + "   TL" + str(tl_id) + "_OFFSET = " + str(hex(tl_offset_list[i])) + ";\n"
                def_line = def_line + "   TL" + str(tl_id) + "_NUM_LINES = " + str(tl_num_lines_list[i]) + ";\n";
                def_line = def_line + "   TL" + str(tl_id) + "_STRIDE = " + str(hex(tl_stride_list[i])) + ";\n";
                def_line = def_line + "   break;\n"            
                tl_configs_hf.write(def_line)
        elif output_type == "array":
            for i in range(0,32):
                def_line = str(hex(tl_coop_csr_list[i])).rjust(10) + "," + str(tl_coop_list[i]).rjust(6) + "," + str(tl_tmask_list[i]).rjust(7) + "," + str(tl_code_list[i]).rjust(6) + "," + \
                           str(tl_scp_start_line_list[i]).rjust(9) + "," + str(tl_tenb_list[i]).rjust(7) + "," + str(hex(tl_addr_list[i])).rjust(11) + "," + str(hex(tl_offset_list[i])).rjust(10) + "," + \
                           str(tl_num_lines_list[i]).rjust(8) + "," + str(hex(tl_stride_list[i])).rjust(9) + ",  //  Min" + str(sh*32 + i) + "-Iter"+str(num_iter)+"\n"
                tl_configs_hf.write(def_line)

        full_tl_tenb_list = full_tl_tenb_list + tl_tenb_list
        full_tl_num_lines_list = full_tl_num_lines_list + tl_num_lines_list
        
        min_coop_id[sh] = (min_coop_id[sh] + num_cooploads_in_shire) % 16

    if num_iter == total_iter-1:        
            tl_configs_hf.write("};\n")
    
    tl_configs_hf.close()
    return (min_coop_id, full_tl_tenb_list, full_tl_num_lines_list)


# TBD: TenB Load and FMA snippet -- look like code duplication and the test may some times over wtire it or add more instructions in the middle.
def generate_tfmas(shires, tfma_tenb_list, tfma_acols_list, target_dir, output_type, num_iter, total_iter):
    
    num_minions = shires * 32

    tfma_tmask_list = [random.randint(0,1) for i in range(num_minions)]
    
    tfma_bcols_list = [random.randint(0,3) for i in range(num_minions)]

    tfma_arows_list = [random.randint(0,15) for i in range(num_minions)]
    
    # When we use Tenb this will be matched to the Tensor Load TenB acols (b rows)
    # The test needs to make these two match    
    #tfma_acols_list = [random.randint(0,15) for i in range(num_minions)]
    
    tfma_astart_col_list = [random.randint(0,15) for i in range(num_minions)]
    
    # This can be overwtiten by the test to enable TenB
    # tfma_tenb_list = [0] * num_minions
    
    # FMA types (FMA32, IMA8A32, FMA16A32)
    tfma_type_fma16a32 = 1
    tfma_type_ima8a32 = 3
    tfma_type_fma32 = 0

    tfma_types_map = {0: tfma_type_fma32,
                      1: tfma_type_fma16a32,
                      2: tfma_type_ima8a32 }

    tfma_type_list = [tfma_types_map[random.randint(0,2)] for i in range(num_minions)]

    tfma_scp_start_linea_list = [random.randint(0,63) for i in range(num_minions)]
    tfma_scp_start_lineb_list = [random.randint(0,63) for i in range(num_minions)]
    
    tfma_clear_rf_list = [random.randint(0,1) for i in range(num_minions)]
    #tfma_configs_hf = open(target_dir+"/tfma_configs.h", "w")
    #tfma_configs_hf.write("switch (minion_id) {\n")

    if num_iter == 0:
        tfma_configs_hf = open(target_dir+"/tfma_configs.h", "w")
        if output_type == "switch":
            def_line = "switch (minion_id) {\n"
        else:
            def_line = "static uint64_t tfma_configs["+str(total_iter*num_minions*13)+"] = {\n"
            def_line = def_line + "// TMASK  BCOLS  AROWS  ACOLS  ASTART_COL  TENB  SCP_START_LINEA  SCP_START_LINEB  CLEAR_RF  TYPE  USE_TENC  UNS_A  UNS_B\n"
        tfma_configs_hf.write(def_line)                          
    else:
        tfma_configs_hf = open(target_dir+"/tfma_configs.h", "a+")
    
    if (output_type == "switch"):
        for i in range(num_minions):
        
            def_line = " case " + str(i) + ":\n   TFMA_TMASK = " + str(tfma_tmask_list[i]) + ";\n"
            def_line = def_line + "   TFMA_BCOLS = " + str(tfma_bcols_list[i]) + ";\n"
            def_line = def_line + "   TFMA_AROWS = " + str(tfma_arows_list[i]) + ";\n"
            if tfma_tenb_list[i] == 0:
                tfma_acols_list[i] = random.randint(0,15)
            def_line = def_line + "   TFMA_ACOLS = " + str(tfma_acols_list[i]) + ";\n"
            def_line = def_line + "   TFMA_ASTART_COL = " + str(tfma_astart_col_list[i]) + ";\n"
            def_line = def_line + "   TFMA_TENB = " + str(tfma_tenb_list[i]) + ";\n"
            def_line = def_line + "   TFMA_SCP_START_LINEA = " + str(tfma_scp_start_linea_list[i]) + ";\n"
            def_line = def_line + "   TFMA_SCP_START_LINEB = " + str(tfma_scp_start_lineb_list[i]) + ";\n"
            def_line = def_line + "   TFMA_CLEAR_RF = " + str(tfma_clear_rf_list[i]) + ";\n"
            def_line = def_line + "   TFMA_TYPE = " + str(tfma_type_list[i]) + ";\n"
            tfma_use_tenc = 0
            tfma_unsigneda = 0
            tfma_unsignedb = 0
            if (tfma_type_list[i] == tfma_type_ima8a32):
                tfma_use_tenc = random.randint(0,1)
                tfma_use_signeda = random.randint(0,1)
                tfma_use_signedb = random.randint(0,1)
            def_line = def_line + "   TFMA_USE_TENC = " + str(tfma_use_tenc) + ";\n"
            def_line = def_line + "   TFMA_UNSIGNEDA = " + str(tfma_unsigneda) + ";\n"
            def_line = def_line + "   TFMA_UNSIGNEDB = " + str(tfma_unsignedb) + ";\n"
            def_line = def_line + "   break;\n" 
            tfma_configs_hf.write(def_line)
                
        tfma_configs_hf.write("}\n")    
    
    elif output_type == "array":
        for i in range(num_minions):
            if tfma_tenb_list[i] == 0:
                tfma_acols_list[i] = random.randint(0,15)
            tfma_use_tenc = 0
            tfma_unsigneda = 0
            tfma_unsignedb = 0
            if (tfma_type_list[i] == tfma_type_ima8a32):
                tfma_use_tenc = random.randint(0,1)
                tfma_use_signeda = random.randint(0,1)
                tfma_use_signedb = random.randint(0,1)
            def_line = str(tfma_tmask_list[i]).rjust(6) + "," + str(tfma_bcols_list[i]).rjust(6) + "," + str(tfma_arows_list[i]).rjust(6) + "," + str(tfma_acols_list[i]).rjust(6) + "," + \
                       str(tfma_astart_col_list[i]).rjust(9) + "," + str(tfma_tenb_list[i]).rjust(7) + "," +  str(tfma_scp_start_linea_list[i]).rjust(12) + "," + str(tfma_scp_start_lineb_list[i]).rjust(15) + "," + \
                       str(tfma_clear_rf_list[i]).rjust(12) + "," + str(tfma_type_list[i]).rjust(7) + "," + str(tfma_use_tenc).rjust(7) + "," + str(tfma_unsigneda).rjust(8) + "," + str(tfma_unsignedb).rjust(5) + "," + \
                       "  // Min" + str(i) + "-Iter"+str(num_iter)+"\n"
            tfma_configs_hf.write(def_line)

        if num_iter == total_iter - 1:
            tfma_configs_hf.write("};")            
            
                    
def generate_mappings(shires, target_dir):

    num_minions = shires * 32

    minion_list  = list(range(0,num_minions))
    #print(minion_list)
    init_addr_list = random.sample(minion_list,num_minions)
    store_list = random.sample(minion_list,num_minions)
    #print("\n",store_list)
    load_list = random.sample(minion_list,num_minions)
    #print("\n",load_list)
    
    start_reg_list = [random.randint(0,31) for i in range(num_minions)]
    #print(start_reg_list)

    reg_stride_list = [random.randint(0,3) for i in range(num_minions)]
    #print(reg_stride_list)

    num_rows_list = [random.randint(0,15) for i in range(num_minions)]
    #print(num_rows_list)
    
    row_size_list = [random.randint(0,2) for i in range(num_minions)]
    row_size_list = [3 if x == 2 else x for x in row_size_list]
    #print(row_size_list)
    
    reduce_op_map = { 0: 0,
                      1: 2,
                      2: 3,
                      3: 4,
                      4: 6,
                      5: 7,
                      6: 8 }

    # Reduce pairs -- since there is interaction involved, this is more convoluted...
    reduce_order = random.sample(minion_list, num_minions)
    reduce_start_reg =  [random.randint(0,31) for i in range(num_minions)]
    reduce_op = [reduce_op_map[random.randint(0,6)] for i in range(num_minions)]
    reduce_config = [0] * num_minions
    reduce_num_regs = [0] * num_minions
    reduce_partner = [0] * num_minions
    #print(reduce_order)
    for m in reduce_order:
        reduce_idx = reduce_order.index(m)
        # Even number is sender and odd is receiver
        if (reduce_idx % 2) == 0:
            reduce_config[m] = 0
            reduce_partner[m] = reduce_order[reduce_idx+1]
            reduce_num_regs[m] = random.randint(0,127)            
        else:
            reduce_config[m] = 1
            reduce_partner[m] = reduce_order[reduce_idx-1]
            reduce_num_regs[m] = reduce_num_regs[reduce_partner[m]]                            
    
    mappings_hf = open(target_dir+"/load_store_mappings.h","w")
    def_line = "switch(minion_id) {\n"
    mappings_hf.write(def_line)
    for i in range(0,num_minions):
        def_line = " case " + str(i) + ":\n";
        def_line = def_line + "   INDEX_STORE = " + str(store_list[i]) + ";\n"
        def_line = def_line + "   INDEX_LOAD = " + str(load_list[i]) + ";\n"
        def_line = def_line + "   INDEX_START_REG = " + str(start_reg_list[i]) + ";\n"
        def_line = def_line + "   INDEX_REG_STRIDE = " + str(reg_stride_list[i]) + ";\n"
        def_line = def_line + "   INDEX_NUM_ROWS = " + str(num_rows_list[i]) + ";\n"
        def_line = def_line + "   INDEX_ROW_SIZE = " + str(row_size_list[i]) + ";\n"
        #def_line = def_line + "   INDEX_REDUCE_ACTION = " + str(reduce_config[i]) + ";\n"
        #def_line = def_line + "   INDEX_REDUCE_PARTNER = " + str(reduce_partner[i]) + ";\n"
        #def_line = def_line + "   INDEX_REDUCE_START_REG = " + str(reduce_start_reg[i]) + ";\n"
        #def_line = def_line + "   INDEX_REDUCE_OP = " + str(reduce_op[i]) + ";\n"
        #def_line = def_line + "   INDEX_REDUCE_NUM_REGS = " + str(reduce_num_regs[i]) + ";\n"

        for j in range(0,num_minions):
            if load_list[j] == store_list[i]:
                def_line = def_line + "   PAIR = " + str(j) + ";\n   break;\n"
                break        
        mappings_hf.write(def_line)    
    mappings_hf.write("}\n")  

def generate_tload_tfma(shires, repeat, target_dir, output_type):

    # For the time being repeat is assumed to be 1
    # When > 1 use a loop
    
    # We need one file per tensor load call
    max_coop_id = [0] * shires
    for it in range(repeat):
        max_coop_id, _, _ = generate_tensorloads(shires, 0, target_dir, output_type, it, repeat, 2, max_coop_id, False)
        max_coop_id, tenb_list, tfma_acols_list = generate_tensorloads(shires, 1, target_dir, output_type, it, repeat, 2, max_coop_id, True)
        
        #print(tfma_acols_list)
        generate_tfmas(shires, tenb_list, tfma_acols_list, target_dir, output_type, it, repeat)
    

parser = argparse.ArgumentParser()
parser.add_argument("--seed", type=int, help="Use seed for random generation if provided, otherwise create your own seed", default=-1)
parser.add_argument("--shires", type=int, help="Now many shires", default = 32)
parser.add_argument("--scenario", type=str, help="What scenario are you trying to generate ?", default = "tload_tstore")
parser.add_argument("--repeat", type=int, help="How many tiems do you want to repeat this scenario ?", default = 1)
parser.add_argument("--target_dir", type=str, help="Target directory where output files are store", default = ".")
parser.add_argument("--output_type", type=str, help="What will be the output type to be fed into test (array / switch)", default = "switch")

args = parser.parse_args()

if args.seed == -1:
    currentDT = time.localtime()
    initial_seed = int(time.mktime(currentDT))
    #print(initial_seed)
    random.seed(initial_seed)
    #print(initial_seed, int(initial_seed))
    #generate_tensorloads(args.shires)
    #generate_tfmas(args.shires)
    #generate_mappings(args.shires)
    if args.scenario == "tload_tfma":
        generate_tload_tfma(args.shires, args.repeat, args.target_dir, args.output_type)
    elif args.scenario == 'tload_tstore':
        generate_mappings(args.shires, args.target_dir, args.output_typ)    
    seed_file = open("tensor_rand_seed","w")
    seed_file.write(str(initial_seed))

else:
    if args.scenario == "tload_tfma":
        generate_tload_tfma(args.shires, args.repeat, args.target_dir, args.output_type)
    elif args.scenario == 'tload_tstore':
        generate_mappings(args.shires, args.target_dir)
