/*
  Examples of using et_abort api
  Below is an example which demonstartes how to abort a kernel execution if parameters
  are not valid.
*/

/* Include api specific header */
#include "utils.h"

/* Define an example structure */
typedef struct {
    int data;
    int length
} Parameters;

int64_t main(const Parameters *const kernel_params_ptr)
{
    if (kernel_params_ptr == NULL)
    {
        /* Abort on bad arguments */
        et_abort();
    }

    return 0;
}