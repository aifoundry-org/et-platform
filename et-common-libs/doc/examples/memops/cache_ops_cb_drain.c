/*
  Example of using cache_ops_cb_drain api.
  Below is an example which demonstrates how to.
*/

/* Include api specific header */
#include <etsoc/isa/cacheops-umode.h>

int main(void)
{
    /* Calculate shire ID and minion ID to pass to API*/
    uint32_t hart = get_hart_id();
    uint32_t minionId = hart >> 1;
    uint32_t shireId = minionId >> 5;
    minionId = minionId & 0x1F;

    /* Drains CB to L3 */
    cache_ops_cb_drain(shireId, minionId);

    return 0;
}