/*
  Examples of using et_assert api.
  Below is an example which demonstartes how to assert an expression. Input parameters are
  validated by assertion and status is also checked with an assert.
*/

/* Include api specific header */
#include "utils.h"

int main(void *input_param)
{
    int status = 0;

    /* Check if pointer is valid */
    et_assert(input_param == NULL);

    /* Call an api and check status*/
    status = init_module();

    /* Check if status is success */
    et_assert(status == 0);

    return 0;
}