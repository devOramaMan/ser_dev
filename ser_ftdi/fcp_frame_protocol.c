

#include "fcp_frame_protocol.h"
#include "protocol_common.h"
#include "func_common.h"
#include <stddef.h>
#include "diagnostics_util.h"
#include <stdio.h>
#include <config.h>

/** Defines ----------------------------------------------------------------*/

/** @brief Index of FCP frame error code */
#define ERRCODE_IDX       2
/** @brief Size of the Header of an FCP frame */
#define FCP_HEADER_SIZE       2
/** @brief Size of the Control Sum (actually not a CRC) of an FCP frame */
#define FCP_CRC_SIZE          1
/** @brief Maximum size of an FCP frame, all inclusive */
#define FCP_MAX_FRAME_SIZE    (FCP_HEADER_SIZE + FCP_MAX_PAYLOAD_SIZE + FCP_CRC_SIZE)
/** @brief FCP Acknowlage code (success msg) */
#define FCP_CODE_ACK  0xF0
/** @brief FCP Acknowlage Failed code (unsuccess msg) */
#define FCP_CODE_NACK 0xFF
/** @brief OK FCP MESSAGE  */
#define FCP_ACK_MSG 0x00

Protocol_t * init_fcp(RX_TX_CODE_FUNC RxSignal, RX_TX_FUNC TxQueue);

int32_t fcp_frame_parse(uint8_t *Buffer, uint32_t *size, uint8_t *rx_buffer, uint8_t *rx_size, uint8_t * code);

int32_t fcp_receive( Protocol_t * pHandle, uint8_t *Buffer, uint32_t * Size );


typedef struct FCP_Handle 
{
  Protocol_t _super;
  uint8_t RxBuffer[FCP_MAX_PAYLOAD_SIZE];
  uint8_t RxSize; 
  uint8_t err_code;
  RX_TX_CODE_FUNC RxSignal;
  RX_TX_FUNC TxQueue;
} FCP_Handle_t;

uint8_t fcp_code[] = { FCP_CODE_ACK, FCP_CODE_NACK };

FCP_Handle_t fcp_handle =
{
  ._super.Code = fcp_code,
  ._super.Size = sizeof(fcp_code),
  ._super.Diag = { 0,0,0,0,0, false },
  ._super.MsgCnt = 0,
  .RxSignal = NULL,
  .TxQueue = NULL  
};

Protocol_t * init_fcp(RX_TX_CODE_FUNC RxSignal, RX_TX_FUNC TxQueue)
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

inline int32_t fcp_frame_parse(uint8_t *Buffer, uint32_t *size, uint8_t *rx_buffer, uint8_t *rx_size, uint8_t * code)
{
  uint8_t crc, i;
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

    #if (DEBUG_FCP_MSG == 1)
      uint8_t * ptr = Buffer;
      for (i = 0; i < pFrame->Size + 3; i++)
      {
        printf("0x%02X ", *ptr);
        ptr++;
      }
      printf("\n");
    #endif

    crc = pFrame->Buffer[pFrame->Size];

    if (crc != fcp_calc_crc(pFrame))
    {
      return PROTOCOL_CODE_CRC_ERROR;
    }

    if(pFrame->Code == FCP_CODE_ACK)
    {
      *rx_size = pFrame->Size;
      for(i = 0; i < pFrame->Size; i++)
        rx_buffer[i] = pFrame->Buffer[i];
      *code = FCP_ACK_MSG;
    }
    else
    {
      *rx_size = 0;
      *code = pFrame->Buffer[0];
    }

    // TODO something with the response
    // payload = bytearray(incomming[STM.PAYLOAD_IDX:payloadLen+STM.PAYLOAD_IDX])
    if( (pFrame->Size + FCP_HEADER_SIZE + 1) < lsize)
    {
      Buffer = &Buffer[pFrame->Size + FCP_HEADER_SIZE + 1];
      *size -= pFrame->Size + FCP_HEADER_SIZE + 1;
    }
    else
    {
      Buffer = NULL;
      *size = 0;
    }

    

    return PROTOCOL_STATUS_OK;
  }
  return 0;
}

int32_t fcp_receive(Protocol_t *pHandle, uint8_t *buffer, uint32_t *size)
{
  uint32_t lsize = *size;
  uint8_t *pBuffer = buffer;
  int32_t ret = PROTOCOL_STATUS_INVALID_CODE;
  FCP_Handle_t * pFcpHandle = (FCP_Handle_t*) pHandle;

  if (pBuffer && (lsize > 0))
  {
    ret = fcp_frame_parse(pBuffer, &lsize, pFcpHandle->RxBuffer, &pFcpHandle->RxSize, &pFcpHandle->err_code);
    if (!ret)
    {
      ret = PROTOCOL_STATUS_OK;
      DiagMsg(DIAG_DEBUG,"Valid FCP MSG (code %d)", pFcpHandle->err_code); //NB a valid msg can also be a nack...
      pFcpHandle->RxSignal(pFcpHandle->RxBuffer, (uint32_t)pFcpHandle->RxSize, pFcpHandle->err_code);      
      pHandle->Diag.MSG_OK++;
    }
    else
    {
      if(ret == PROTOCOL_CODE_CRC_ERROR)
      {
        pFcpHandle->RxSignal(pFcpHandle->RxBuffer, (uint32_t)pFcpHandle->RxSize, (uint32_t) PROTOCOL_CODE_CRC_ERROR);
        pHandle->Diag.CRC_ERROR++;
      }
      
      if(ret == PROTOCOL_STATUS_INVALID_CODE)
        pHandle->Diag.UNKNOWN_MSG++;
      
      pHandle->Diag.RX_ERROR++;
    }
  }
  buffer = pBuffer;
  *size = lsize;
  return ret;
}


int32_t fcp_send_async(int32_t reg, int32_t frameid, int32_t motor_select, uint8_t * value, int32_t size)
{
  int32_t ret, i;
  FCP_Frame_t msg;
  int32_t crc_idx = size + 1;
#if ( DEBUG_FCP_MSG == 1 )
  uint8_t * ptr = (uint8_t*)&msg;
#endif

  msg.Code = (motor_select << 5);
  msg.Code |= frameid & ~(0xE0);
  msg.Buffer[0] = (uint8_t)reg;
  msg.Size = crc_idx;
  for(i = 0; i < size; i++)
    msg.Buffer[i+1] = value[i];
  msg.Buffer[crc_idx] = fcp_calc_crc(&msg);

#if ( DEBUG_FCP_MSG == 1 )
  for(i=0;i < msg.Size + 3 ; i++ )
  {
    printf("0x%02X ", *ptr);
    ptr++;
  }

  printf("\n");
#endif

  ret = fcp_handle.TxQueue((uint8_t*)&msg, (msg.Size + 3));

  return ret;
}
