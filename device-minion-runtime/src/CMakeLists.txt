# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0

get_filename_component(SHARED_INC_DIR "shared/minion_rt_helpers/include" ABSOLUTE)

add_subdirectory(shared)
add_subdirectory(MachineMinion)
add_subdirectory(MasterMinion)
add_subdirectory(WorkerMinion)
