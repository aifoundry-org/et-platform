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
/*! \fn void sp_intf_init(void)
    \brief This function initializes service processor interface
    \param None
    \return Status indicating success or negative error
*/
void sp_intf_init(void);

/*! \fn void sp_intf_init(void)
    \brief This function starts command dispatcher
    \param None
    \return Status indicating success or negative error
*/
void launch_command_dispatcher(void);
