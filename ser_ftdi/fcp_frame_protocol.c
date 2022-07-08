

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

/** @brief OK FCP MESSAGE  */
#define FCP_ACK_MSG 0x00

int32_t fcp_frame_parse(uint8_t **Buffer, uint32_t *size, uint8_t *rx_buffer, uint8_t *rx_size, uint8_t * code);


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

inline int32_t fcp_frame_parse(uint8_t **Buffer, uint32_t *size, uint8_t *rx_buffer, uint8_t *rx_size, uint8_t * code)
{
  uint8_t crc, i;
  uint32_t lsize = *size;
  uint8_t *pBuffer = *Buffer;
  FCP_Frame_t * pFrame = (FCP_Frame_t*) pBuffer;

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
      uint8_t * ptr = *pBuffer;
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
      *Buffer = &pBuffer[pFrame->Size + FCP_HEADER_SIZE + 1];
      *size -= pFrame->Size + FCP_HEADER_SIZE + 1;
    }
    else
    {
      *Buffer = NULL;
      *size = 0;
    }

    

    return PROTOCOL_STATUS_OK;
  }
  return 0;
}

int32_t fcp_receive(Protocol_t *pHandle, uint8_t **buffer, uint32_t *size)
{
  int32_t ret = PROTOCOL_STATUS_INVALID_CODE;
  FCP_Handle_t * pFcpHandle = (FCP_Handle_t*) pHandle;
  RX_TX_CODE_FUNC RxSignal = (RX_TX_CODE_FUNC)pFcpHandle->RxSignal;

  if ( *buffer && (*size > 0) )
  {
    ret = fcp_frame_parse(buffer, size, pFcpHandle->RxBuffer, &pFcpHandle->RxSize, &pFcpHandle->err_code);
    if (!ret)
    {
      ret = PROTOCOL_STATUS_OK;
      DiagMsg(DIAG_DEBUG,"Valid FCP MSG (code %d)", pFcpHandle->err_code); //NB a valid msg can also be a nack...
      RxSignal(pFcpHandle->RxBuffer, (uint32_t)pFcpHandle->RxSize, pFcpHandle->err_code);      
      pHandle->Diag.MSG_OK++;
    }
    else
    {
      if(ret == PROTOCOL_CODE_CRC_ERROR)
      {
        RxSignal(pFcpHandle->RxBuffer, (uint32_t)pFcpHandle->RxSize, (uint32_t) PROTOCOL_CODE_CRC_ERROR);
        pHandle->Diag.CRC_ERROR++;
      }
      else if(ret == PROTOCOL_STATUS_INVALID_CODE)
        pHandle->Diag.UNKNOWN_MSG++;
      
      if( ret == PROTOCOL_STATUS_UNDERFLOW)
        pHandle->Diag.UNDERFLOW++;
      else
        pHandle->Diag.RX_ERROR++;
               
    }
  }
  return ret;
}

