    // Barrier used (16 to 23)
    // 8 minion barrier, starting at 16th barrier
    uint64_t barrier_o_t = (7 << 5) + 16;

    // A matrix is 128 x 1024 elements logically
    // Each shire works with a section of 64 x 1024 (rows x cols)
    // Within a shire, even minions work with first 512 cols and odd minions second 512 cols
    // We have 4 minions (even prefetch minions) prefetching the first 512 cols of 64 rows
    // and 4 other minions (odd prefetch minions) prefetching the second 512 cols of 64 rows

    // A row offset computation
    // Within a shire, each minion pair advances 16 rows
    uint64_t  aRowInitMinion = (minion_id / 2) * 16;
    // Each pair of consecutive shires work with different A section
#if N_SHIRES_COMPUTE == 32
    uint64_t  aRowInitShire  = (shire_id & 1) * 64;
#else
    uint64_t  aRowInitShire  = 0;
#endif
    // Total is offset due shire and offset due minion within shire
    uint64_t  aRowInit       = aRowInitMinion + aRowInitShire;
    // Each A Row pitch is 1056 elements of 2 bytes
    uint64_t  aRowPitch      = 1056 * 2;
    uint64_t  aRowOffset     = aRowInit * aRowPitch;

    // A col offset computation
    // Gets if even/odd and based on that offsets 512 cols
    // Each A Col pitch is 2 bytes
    uint64_t  aColPitch  = 2;

    // Activation is stored in the L2 SCP using the "linear" region
#if N_SHIRES_COMPUTE == 32
    uint64_t  tensor_a_init = (uint64_t volatile) 0x00C0000000ULL + (L2_SCP_ACTIVATION_SOURCE_OFFSET << 11) + aRowOffset;
#else
    uint64_t  tensor_a_init = (uint64_t volatile) 0x0080000000ULL + (shire_id << 23) + (L2_SCP_ACTIVATION_SOURCE_OFFSET << 6) + aRowOffset;
#endif

    // Activation minions use L2 scp entries to keep prefetched data
    // Each minion prefetches 16 iterations each one of 16 cachelines, total of 256 cachelines
    // Each one is storing data 256 cachelines apart
    uint64_t  l2_scp_dest_entry_logical = (minion_id * 256) + L2_SCP_ACTIVATION_PREF_OFFSET;
    // Dest entry goes to bits { 62:48, 5:4 } in TensorLoadL2Scp instruction
    uint64_t  l2_scp_dest_entry = ((l2_scp_dest_entry_logical & 0xFFFFFFFCULL) << 46) + ((l2_scp_dest_entry_logical & 3ULL) << 4);

    // What advances on every tensor load L2 Scp iteration
    // Advances 16 entries the l2 scp destination
    uint64_t  l2_scp_dest_adv_logical = 16;
    uint64_t  l2_scp_dest_adv = ((l2_scp_dest_adv_logical & 0xFFFFFFFCULL) << 46) + ((l2_scp_dest_adv_logical & 3ULL) << 4);
    // Advances 32 cols the A pointer
    uint64_t  tensor_a_adv = 32 * aColPitch;
    uint64_t  tensor_load_l2scp_adv = l2_scp_dest_adv + tensor_a_adv;

    // Stride for TensorLoad (one a row)
    uint64_t  tensor_load_l2scp_stride = aRowPitch;

    // Waits for sync minions to report that data is ready to be consumed
    fcc(FCC_1);

    __asm__ __volatile__ (
        // Generate TensorLoadL2Scp command for matrix A
        "or     x7, %[l2_scp_dest_entry], %[tensor_a_init]\n" // Merge source with l2 scp dest
        "addi   x7, x7, 15\n"                                 // 16 cachelines worth of prefetching
        // TensorWait ID for TensorLoadL2Scp
        "li     x4, 2\n"
        // TensorLoadL2Scp stride
        "addi   x31, %[tensor_load_l2scp_stride], 0x0\n"
        // Prefetches to wait before doing a tensor wait
        "li     x5, 1\n"
        // FCC0 addr + data to compute threads
        "li     x30, 0x13ff400c0\n"
        "li     x28, %[compute_threads]\n"

        // Open loop for dimension 1
        // Reset Loop counter of Dim: 1
        "li     x6, 16\n"
            "1:\n"
            // Execute A Prefetch and consumes credit
            "csrw   0x821, zero\n"
            "csrw   0x85F, x7\n"

            // Swap the A prefetch ID
            "xori   x31, x31, 1\n"

            // Checks if needs to do a tensor wait
            "bne    x5, zero, 2f\n"
                // Switches to another barrier to prevent races between threads reusing
                // same barrier before the last one is done
                "addi   %[barrier_o_t], %[barrier_o_t], 1\n"
                "andi   %[barrier_o_t], %[barrier_o_t], -9\n"

                // If so, does tensor wait and switches ID to wait for
                "csrw   0x830, x4\n"
                "xori   x4, x4, 1\n"

                // Access to barrier, if last sends FCC0 to compute minions
                "csrrw  x29, 0x820, %[barrier_o_t] \n"
                "beq    x29, zero, 3f\n"
                    "sd     x28, 0(x30)\n"
                "j 3f\n"

            // Decrements iterations to skip tensor wait
            "2:\n"
                "addi   x5, x5, -1\n"

            // Done prefetch
            "3:\n"
            // Increments 16 positions the destination in L2 Scp
            "add    x7, x7, %[tensor_load_l2scp_adv]\n"
            // Increments inner loop count and checks if end
            "addi   x6, x6, -1\n"
            "bne    x6, zero, 1b\n"

        // Sends an FCC to the other thread
        // Switches to another barrier to prevent races between threads reusing
        // same barrier before the last one is done
        "addi   %[barrier_o_t], %[barrier_o_t], 1\n"
        "andi   %[barrier_o_t], %[barrier_o_t], -9\n"
        "csrw   0x830, x4\n"
        "csrrw  x29, 0x820, %[barrier_o_t]\n"
        "beq    x29, zero, 4f\n"
            "sd     x28, 0(x30)\n"
        "4: \n"
      : 
        [tensor_load_l2scp_stride] "+&r" (tensor_load_l2scp_stride) 
      : 
        [compute_threads] "i" (COMPUTE_THREADS),
        [barrier_o_t] "r" (barrier_o_t),
        [tensor_a_init] "r" (tensor_a_init),
        [l2_scp_dest_entry] "r" (l2_scp_dest_entry),
        [tensor_load_l2scp_adv] "r" (tensor_load_l2scp_adv)
      : 
       "x31",
       "x30",
       "x29",
       "x28",
       "x6",
       "x4",
       "x5",
       "x7"
    );

