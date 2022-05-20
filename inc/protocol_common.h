
#include <stdint.h>
#include <stdbool.h>

#ifndef PROTOCOL_COMMON
#define PROTOCOL_COMMON

#define PROTOCOL_STATUS_OK                0x00
#define PROTOCOL_STATUS_TRANSFER_ONGOING  0x01
#define PROTOCOL_STATUS_WAITING_TRANSFER  0x02
#define PROTOCOL_STATUS_INVALID_PARAMETER 0x03
#define PROTOCOL_STATUS_TIME_OUT          0x04
#define PROTOCOL_STATUS_INVALID_FRAME     0x05

#define PROTOCOL_STATUS_UNDERFLOW         0x06
#define PROTOCOL_STATUS_INVALID_CODE      0x07
#define PROTOCOL_STATUS_CRC_ERROR         0x08


typedef struct Protocol_Diag
{
    uint32_t MSG_OK;
    uint32_t RX_ERROR;
    uint32_t TX_ERROR;
    uint32_t CRC_ERROR;
    uint32_t UNKNOWN_MSG;
    bool Flag;
}Protocol_Diag_t;


typedef struct protocol_base
{
  uint8_t * Code;
  uint16_t Size;
  Protocol_Diag_t Diag;
  uint32_t MsgCnt;
} Protocol_t;

typedef int32_t (*PROTOCOL_CALLBACK)( Protocol_t * pHandle, uint8_t * buffer, uint32_t * size );

typedef struct Protocol
{
    Protocol_t * pProtocol;
    PROTOCOL_CALLBACK pCallback;
}Protocol_Handle_t;

#endif 