

#include "fcp_frame_protocol.h"
#include "protocol_common.h"
#include "func_common.h"
#include <stddef.h>
#include "diagnostics_util.h"

/** Defines ----------------------------------------------------------------*/



/** @brief Index of FCP frame error code */
#define ERRCODE_IDX       2
/** @brief Size of the Header of an FCP frame */
#define FCP_HEADER_SIZE       2
/** @brief Maximum size of the payload of an FCP frame */
#define FCP_MAX_PAYLOAD_SIZE  128
/** @brief Size of the Control Sum (actually not a CRC) of an FCP frame */
#define FCP_CRC_SIZE          1
/** @brief Maximum size of an FCP frame, all inclusive */
#define FCP_MAX_FRAME_SIZE    (FCP_HEADER_SIZE + FCP_MAX_PAYLOAD_SIZE + FCP_CRC_SIZE)
/** @brief FCP Acknowlage code (success msg) */
#define FCP_CODE_ACK  0xF0
/** @brief FCP Acknowlage Failed code (unsuccess msg) */
#define FCP_CODE_NACK 0xFF

Protocol_t * init_fcp(PROC_SIGNAL RxSignal, RX_TX_FUNC TxQueue);

int32_t fcp_frame_parse(uint8_t *Buffer, uint32_t * size);

int32_t fcp_receive( Protocol_t * pHandle, uint8_t *Buffer, uint32_t * Size );


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

typedef struct FCP_Handle 
{
  Protocol_t _super;
  PROC_SIGNAL RxSignal;
  RX_TX_FUNC TxQueue;
} FCP_Handle_t;

uint8_t fcp_code[] = { FCP_CODE_ACK, FCP_CODE_NACK };

FCP_Handle_t fcp_handle =
{
  ._super.Code = fcp_code,
  ._super.Diag = { 0,0,0,0,0, false },
  ._super.MsgCnt = 0,
  .RxSignal = NULL,
  .TxQueue = NULL  
};

Protocol_t * init_fcp(PROC_SIGNAL RxSignal, RX_TX_FUNC TxQueue)
{
  fcp_handle.RxSignal = RxSignal;
  fcp_handle.TxQueue = TxQueue;
  return (Protocol_t *)&fcp_handle;
}

uint8_t fcp_calc_crc( FCP_Frame_t * pFrame )
{
  uint8_t nCRC = 0;
  uint16_t nSum = 0;
  uint8_t idx;

  if( NULL != pFrame )
  {
    nSum += pFrame->Code;
    nSum += pFrame->Size;

    for ( idx = 0; idx < pFrame->Size; idx++ )
    {
      nSum += pFrame->Buffer[idx];
    }

    nCRC = (uint8_t)(nSum & 0xFF) ; // Low Byte of nSum
    nCRC += (uint8_t) (nSum >> 8) ; // High Byte of nSum
  }

  return nCRC;
}

inline int32_t fcp_frame_parse(uint8_t *Buffer, uint32_t *size)
{
  uint8_t crc;
  uint32_t lsize = *size;
  FCP_Frame_t * pFrame = (FCP_Frame_t*) Buffer;

  if (lsize < FCP_HEADER_SIZE)
  {
    return PROTOCOL_STATUS_UNDERFLOW;
  }
  else
  {

    if ((pFrame->Code != FCP_CODE_ACK) && (pFrame->Code != FCP_CODE_NACK))
      return PROTOCOL_STATUS_INVALID_CODE;


    if (lsize < pFrame->Size + FCP_HEADER_SIZE)
      return PROTOCOL_STATUS_UNDERFLOW;

    crc = pFrame->Buffer[pFrame->Size];

    if (crc != fcp_calc_crc(pFrame))
    {
      return PROTOCOL_STATUS_CRC_ERROR;
    }

    // TODO something with the response
    // payload = bytearray(incomming[STM.PAYLOAD_IDX:payloadLen+STM.PAYLOAD_IDX])
    if( (pFrame->Size + FCP_HEADER_SIZE + 1) < lsize)
      Buffer = &Buffer[pFrame->Size + FCP_HEADER_SIZE + 1];
    else
      Buffer = NULL;

    *size -= pFrame->Size + FCP_HEADER_SIZE + 1;

    return PROTOCOL_STATUS_OK;
  }
  return 0;
}

int32_t fcp_receive(Protocol_t *pHandle, uint8_t *buffer, uint32_t *size)
{
  uint32_t lsize = *size, nsize;
  uint8_t *pBuffer = buffer;
  int32_t ret = PROTOCOL_STATUS_OK;
  while (pBuffer && (lsize > 0))
  {
    nsize = lsize;
    ret = fcp_frame_parse(pBuffer, &lsize);
    if (ret)
    {
      // TODO Diag
    }

    if (lsize == nsize)
      break;
  }
  buffer = pBuffer;
  *size = lsize;
  return ret;
}


int32_t fcp_write_register_async(int32_t reg, int32_t frameid, int32_t motor_select, int32_t value, int32_t size, Data_Type type)
{
  int32_t ret;
  FCP_Frame_t msg;
  int32_t crc_idx;


  msg.Code = (motor_select << 5);
  msg.Code |= frameid & ~(0xE0);
  msg.Buffer[0] = (uint8_t)reg;
  switch (type)
  {
  case INTEGER8:
  case UNSIGNED8:
    msg.Size = 1;
    msg.Buffer[1] = (uint8_t) value;
    crc_idx = 2;
    break;
  case INTEGER16:
  case UNSIGNED16:
    msg.Size = 2;
    msg.Buffer[1] = (uint8_t) (value >> 8); // MSB
    msg.Buffer[2] = (uint8_t) (value & 0xFF); //LSB
    crc_idx = 3;
    break;
  case INTEGER32:
  case UNSIGNED32:
    msg.Size = 4;
    //TODO Verify endian
    msg.Buffer[2] = (uint8_t) (value >> 16);
    msg.Buffer[1] = (uint8_t) (value & 0xFF00);
    msg.Buffer[3] = (uint8_t) (value >> 8);
    msg.Buffer[4] = (uint8_t) (value & 0xFF);
    crc_idx = 5;
  case REAL32:
  //TODO Verify endian and add float conversion
    msg.Size = 4;
    crc_idx = 5;
    break;
  default:
    DiagMsg(DIAG_ERROR, "Format not supported");
    break;
  }
  msg.Buffer[crc_idx] = fcp_calc_crc(&msg);

  ret = fcp_handle.TxQueue((uint8_t*)&msg, 4);

  return ret;
}

int32_t fcp_read_register_async(int32_t reg, int32_t frameid, int32_t motor_select, Data_Type type)
{
  int32_t ret;
  FCP_Frame_t msg;

  msg.Code = (motor_select << 5);
  msg.Code |= frameid & ~(0xE0);
  msg.Size = 1;
  msg.Buffer[0] = (uint8_t)reg;
  msg.Buffer[1] = fcp_calc_crc(&msg);

  ret = fcp_handle.TxQueue((uint8_t*)&msg, 4);

  return ret;
}
