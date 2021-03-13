#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "sp_otp.h"

/*! \fn int otp_get_chip_revision(char *chip_rev)
    \brief Interface to get the chip revision
    \param *chip_rev  Pointer to chip revision variable
    \returns Status indicating success or negative error
*/
int otp_get_chip_revision(char *chip_rev);