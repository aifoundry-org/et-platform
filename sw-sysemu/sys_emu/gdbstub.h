/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
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
