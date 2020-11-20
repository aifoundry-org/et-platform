#ifndef DEVICE_OPS_API_H
#define DEVICE_OPS_API_H

#include <stdint.h>
#include <esperanto/device-apis/operations-api/device_ops_api_spec.h>
#include <esperanto/device-apis/operations-api/device_ops_api_rpc_types.h>
#include <esperanto/device-apis/device_apis_message_types.h>

/// \brief Handle device operations command from host
/// \param[in] sq_id: Submission queue ID
int64_t handle_device_ops_cmd(const uint32_t sq_idx);

#endif /* DEVICE_OPS_API_H */
