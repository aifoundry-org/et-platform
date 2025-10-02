/***********************************************************************
*
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/

/*! \fn void launch_host_sp_command_handler(void)
    \brief This function launchs Host to SP command handler
    \param None
    \return None
*/
void launch_host_sp_command_handler(void);

/*! \fn void launch_mm_sp_command_handler(void)
    \brief This function launchs Master Minion to SP command handler
    \param None
    \return None
*/
void launch_mm_sp_command_handler(void);

/*! \fn void host_intf_init(void)
    \brief Initialize Host to Service Processor interface
    \param None
    \return Status indicating success or negative error
*/
void host_intf_init(void);

/*! \fn void mm_intf_init(void)
    \brief Initialize Server Processor to Master Minion FW interface
    \param None
    \return Status indicating success or negative error
*/
void mm_intf_init(void);