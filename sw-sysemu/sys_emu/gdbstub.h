/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#ifndef GDBSTUB_H
#define GDBSTUB_H

class sys_emu;
namespace bemu { class System; }

enum gdbstub_status {
    GDBSTUB_STATUS_NOT_INITIALIZED,
    GDBSTUB_STATUS_WAITING_CLIENT,
    GDBSTUB_STATUS_RUNNING
};

int gdbstub_init(sys_emu*, bemu::System*);
int gdbstub_accept_client();
int gdbstub_close_client();
void gdbstub_fini();
int gdbstub_io();
void gdbstub_signal_break(int thread);
enum gdbstub_status gdbstub_get_status();

#endif
