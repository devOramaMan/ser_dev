
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
#define PROTOCOL_STATUS_BUSY              0x08
#define PROTOCOL_STATUS_NACK              0xFF

#define PROTOCOL_CODE_CRC_ERROR           0x80
#define PROTOCOL_CODE_TIMEOUT             0x81
#define PROTOCOL_CODE_MSG_LOSS            0x82
//#define PROTOCOL_CODE_USED_SOCKET_TIMEOUT 0x82



typedef struct Protocol_Diag
{
    uint32_t MSG_OK;
    uint32_t RX_ERROR;
    uint32_t TX_ERROR;
    uint32_t CRC_ERROR;
    uint32_t UNDERFLOW;
    uint32_t UNKNOWN_MSG;
    uint32_t TIMEOUT;
}Protocol_Diag_t;

typedef struct Msg_Keys
{
  uint8_t code;
  uint8_t range;
  uint8_t err_code;
  uint8_t type;
  uint32_t id;
  uint32_t size;
}Msg_Keys_t;

#define MSG_KEY_SIZE sizeof(Msg_Keys_t)

typedef struct Msg_Base
{
  Msg_Keys_t keys;
  uint8_t data[128];
}Msg_Base_t;


typedef struct Protocol_Base
{
  Protocol_Diag_t Diag;
  uint8_t * Code;
  uint16_t Size;
  uint32_t MsgCnt;
} Protocol_t;

typedef enum
{
  INTEGER8 = 2,
  INTEGER16 = 3,
  INTEGER32 = 4,
  UNSIGNED8 = 5,
  UNSIGNED16 = 6,
  UNSIGNED32 = 7,
  REAL32 = 8,
  DATA_TYPE_SIZE,
}Data_Type;

static const char dataTypeStr[DATA_TYPE_SIZE][14] =
{
  "int8_t   : 2",
  "int16_t  : 3",
  "int32_t  : 4",
  "uint8_t  : 5",
  "uint16_t : 6",
  "uint32_t : 7",
  "float    : 8"
  "",
  "",
};

#define FCP_SINGLE_TOPIC     100
#define FCP_MULTIPLE_TOPIC   101

#define FCP_SUBSCRIBE_TOPIC  110

#define READ_FILE_RECORD_TOPIC  120
#define WRITE_FILE_RECORD_TOPIC 121

#define KMC_REC_SUBSCRIBE_TOPIC  150

typedef int32_t (*PROTOCOL_CALLBACK)( Protocol_t * pHandle, uint8_t ** buffer, uint32_t * size );

typedef struct Protocol_Handle
{
    Protocol_t * pProtocol;
    PROTOCOL_CALLBACK pCallback;
}Protocol_Handle_t;

#if defined(__GNUC__) || defined(__clang__)
#define __weak __attribute__((weak)) 
#endif 

#endif
