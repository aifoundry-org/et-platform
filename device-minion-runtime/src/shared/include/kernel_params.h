#ifndef KERNEL_PARAMS_H
#define KERNEL_PARAMS_H

#include <stdint.h>
#include <esperanto/device-api/device_api.h>

#ifdef __cplusplus
// FIXME This header is still included by the runtime, remove the dependency
// at some point
using kernel_params_t = ::device_api::dev_api_kernel_params_t;
#else
// FIXME create an alias to avoid extensive modifications in the existing code-base
typedef struct dev_api_kernel_params_t kernel_params_t;
#endif

#endif
