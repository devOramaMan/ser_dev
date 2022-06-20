


#ifndef FCP_FRAME_PROTOCOL
#define FCP_FRAME_PROTOCOL

#include <stdint.h>
#include "protocol_common.h"


/** @brief Maximum size of the payload of an FCP frame */
#define FCP_MAX_PAYLOAD_SIZE  128

/**
 * @brief This structure contains and formats a Frame Communication Protocol's frame
 */
typedef struct FCP_Frame 
{
  uint8_t Code;                         /**< Identifier of the Frame. States the nature of the Frame. */
  uint8_t Size;                         /**< Size of the Payload of the frame in bytes. */
  uint8_t Buffer[FCP_MAX_PAYLOAD_SIZE]; /**< buffer containing the Payload of the frame. */
  uint8_t FrameCRC;                     /**< "CRC" of the Frame. Computed on the whole frame (Code, */
} FCP_Frame_t;

uint8_t fcp_calc_crc( FCP_Frame_t * pFrame );
int32_t fcp_send_async(int32_t reg, int32_t frameid, int32_t motor_select, uint8_t * value, int32_t size);


#endif /* fcp_frame_protocol.h */
