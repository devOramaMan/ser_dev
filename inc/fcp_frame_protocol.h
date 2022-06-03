


#ifndef FCP_FRAME_PROTOCOL
#define FCP_FRAME_PROTOCOL

#include <stdint.h>
#include "protocol_common.h"


/** @brief Maximum size of the payload of an FCP frame */
#define FCP_MAX_PAYLOAD_SIZE  128

int32_t fcp_send_async(int32_t reg, int32_t frameid, int32_t motor_select, uint8_t * value, int32_t size);

#endif /* fcp_frame_protocol.h */
