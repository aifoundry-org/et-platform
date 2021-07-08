/***********************************************************************
*
* Copyright (C) 2021 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/

#include <stdint.h>
#include "usdelay.h"

#include "FreeRTOS.h"
#include "task.h"

/*
** Implementation for us delay
** This function should only be called after vTaskStartScheduler()
*/
int usdelay(uint32_t usec)
{
  vTaskDelay(usec/portTICK_PERIOD_MS/1000);
  return 0;
}
