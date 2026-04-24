/* Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include "erbium/isa/hart.h"

int main(void)
{
    unsigned int hart_id = get_hart_id();
    (void)hart_id;
    return 0;
}
