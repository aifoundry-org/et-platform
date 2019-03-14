/*-------------------------------------------------------------------------
* Copyright (C) 2019, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

#include <cstdlib>

extern int main_internal(int argc, char * argv[]);

////////////////////////////////////////////////////////////////////////////////
// Main function
// It is kept separate on purpose so that we can produce a libsysemu.a library
// that can be included in other simulator implementations and instantiations
////////////////////////////////////////////////////////////////////////////////

int main(int argc, char * argv[])
{
    return main_internal(argc, argv);
}
