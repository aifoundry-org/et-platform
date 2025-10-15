/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
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