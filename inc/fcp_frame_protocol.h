


#ifndef FCP_FRAME_PROTOCOL
#define FCP_FRAME_PROTOCOL

#include <stdint.h>

/** @brief Maximum size of the payload of an FCP frame */
#define FCP_MAX_PAYLOAD_SIZE  128

typedef enum
{
  INTEGER8 = 2,
  INTEGER16 = 3,
  INTEGER32 = 4,
  UNSIGNED8 = 5,
  UNSIGNED16 = 6,
  UNSIGNED32 = 7,
  REAL32 = 8,
}Data_Type;

int32_t fcp_write_register_async(int32_t reg, int32_t frameid, int32_t motor_select, int32_t value, int32_t size,  Data_Type type);

int32_t fcp_read_register_async(int32_t reg, int32_t frameid, int32_t motor_select, Data_Type type);

#endif /* fcp_frame_protocol.h */
