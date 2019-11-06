#include "kernel_launch_cmd_builder.h"
#include "kernel_launch_cmd_reader.h"
#include "gtest/gtest.h"

#include <cstdint>
#include <vector>

#undef ns
#define ns(x) FLATBUFFERS_WRAP_NAMESPACE(ETDevAPICommands, x)

namespace {

/**
 * Example test where we are constructing and deserializing a command in C
 */
TEST(KernelLaunchCmd, build_c)
{

    void *buf;
    size_t size;
    flatcc_builder_t builder, *B;
    // Initialize the builder object.
    B = &builder;
    flatcc_builder_init(B);

    uint8_t params[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    size_t params_count = sizeof(params) / sizeof(params[0]);
    flatbuffers_uint8_vec_ref_t params_v =
        flatbuffers_uint8_vec_create(B, params, params_count);

    //start root node

    ns(KernelLaunchCommand_start_as_root(B));

    MessageHeader_t header = {
        .message_type = MessageType_Command,
        .command_id = CommandID_LaunchKernel,
        .response_id = ResponseID_Unknown,
        .event_id = EventID_Unknown,
        .time_stamp = 1233424,
        .stream_id = 1,
        .sequence_id = 2,
    };

    ns(KernelLaunchCommand_header_add(B, &header));
    ns(dim3_t) gidDim;
    ns(dim3_assign(&gidDim, 1, 2, 3));
    ns(KernelLaunchCommand_gidDim_add(B, &gidDim));
    ns(dim3_t) blockDim;
    ns(dim3_assign(&blockDim, 4, 5, 6));
    ns(KernelLaunchCommand_blockDim_add(B, &blockDim));
    ns(KernelLaunchCommand_kernel_pc_add(B, 0xdeadbeef));

    ns(KernelLaunchCommand_params_add(B, params_v));
    // End root node
    ns(KernelLaunchCommand_end_as_root(B));

    // get pointer to serialized buffer
    buf = flatcc_builder_finalize_buffer(B, &size);

    //Now decode the buffer and access its elements
    ns(KernelLaunchCommand_table_t) kernel_launch = ns(KernelLaunchCommand_as_root(buf));

    MessageHeader_struct_t header_dec = ns(KernelLaunchCommand_header(kernel_launch));
    ASSERT_EQ(header_dec->command_id, CommandID_LaunchKernel);

    ns(dim3_struct_t) gidDim_dec = ns(KernelLaunchCommand_gidDim(kernel_launch));
    ASSERT_EQ(gidDim_dec->x, 1);
    ASSERT_EQ(gidDim_dec->z, 3);

    ns(dim3_struct_t) blockDim_dec = ns(KernelLaunchCommand_blockDim(kernel_launch));
    ASSERT_EQ(blockDim_dec->x, 4);
    ASSERT_EQ(blockDim_dec->z, 6);

    uint64_t kernel_pc = ns(KernelLaunchCommand_kernel_pc(kernel_launch));
    ASSERT_EQ(kernel_pc, 0xdeadbeef);

    flatbuffers_uint8_vec_t params_dec = ns(KernelLaunchCommand_params(kernel_launch));
    size_t params_len = flatbuffers_uint8_vec_len(params_dec);

    ASSERT_NE(params_dec, nullptr);
    ASSERT_EQ(params_len, 10);
    ASSERT_EQ(flatbuffers_uint8_vec_at(params_dec, 4), 4);


    free(buf);
    flatcc_builder_clear(B);
}

};
