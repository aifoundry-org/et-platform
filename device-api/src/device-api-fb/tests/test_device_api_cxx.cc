#include "kernel_launch_cmd_generated.h"
#include "gtest/gtest.h"

#include <cstdint>
#include <vector>

namespace {

using namespace ETDevAPICommands;

/**
 * Example test where we are constructing and deserialzing command in C++
 */
TEST(KernelLaunchCmd, build_cxx)
{
    flatbuffers::FlatBufferBuilder builder(1024);

    // Note the vector need sto be constructed outside constructing
    // the table
    uint8_t params_vec [] = { 1, 2, 3, 4, 5, 6, 7};
    auto params = builder.CreateVector(params_vec, sizeof(params_vec)/sizeof(params_vec[0]));

    KernelLaunchCommandBuilder kernel_launch(builder);

    auto header = MessageHeader {
        MessageType::Command,
        CommandID::LaunchKernel,
        ResponseID::Unknown, // response_id
        EventID::Unknown, // event_id
        1233424, // time_stamp
        1, // stream_id
        2, // sequence_id
    };
    kernel_launch.add_header(&header);

    auto gidDim = dim3{ 1, 2, 3};
    kernel_launch.add_gidDim(&gidDim);

    auto blockDim = dim3 { 4, 5, 6};
    kernel_launch.add_blockDim(&blockDim);

    kernel_launch.add_kernel_pc(0xdeadbeef);
    kernel_launch.add_params(params);
    auto kernel_launch_command = kernel_launch.Finish();
    builder.Finish(kernel_launch_command);

    uint8_t *buf = builder.GetBufferPointer();
    int size = builder.GetSize();


    auto cmd_dec = GetKernelLaunchCommand(buf);
    ASSERT_EQ(cmd_dec->header()->command_id(), CommandID::LaunchKernel);
    auto gidDim_dec = *cmd_dec->gidDim();
    ASSERT_EQ(gidDim_dec.x(), 1);
    ASSERT_EQ(gidDim_dec.z(), 3);

    auto blockDim_dec =*cmd_dec->blockDim();
    ASSERT_EQ(blockDim_dec.x(), 4);
    ASSERT_EQ(blockDim_dec.z(), 6);

    ASSERT_EQ(cmd_dec->kernel_pc(), 0xdeadbeef);

    auto params_dec = cmd_dec->params();
    ASSERT_NE(params_dec, nullptr);
    ASSERT_EQ((*params_dec)[3], 4);
}

};
