

#include "fcp_frame_protocol.h"
#include "util_common.h"
#include <stddef.h>

/* Defines ----------------------------------------------------------------*/

/** @brief Index of FCP frame payload length */
#define PAYLOADLEN_IDX    1
/** @brief Index of FCP frame error code */
#define ERRCODE_IDX       2
/** @brief Index of FCP frame payload */
#define PAYLOAD_IDX       2
/** @brief Offset of the CRC from payload length*/
#define CRC_OFFSET        2

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


int32_t fsp_frame_parse(uint8_t *Buffer, uint32_t * size);


/**
 * @brief This structure contains and formats a Frame Communication Protocol's frame
 */
typedef struct FCP_Frame_s 
{
  uint8_t Code;                         /**< Identifier of the Frame. States the nature of the Frame. */
  uint8_t Size;                         /**< Size of the Payload of the frame in bytes. */
  uint8_t Buffer[FCP_MAX_PAYLOAD_SIZE]; /**< buffer containing the Payload of the frame. */
  uint8_t FrameCRC;                     /**< "CRC" of the Frame. Computed on the whole frame (Code, */
} FCP_Frame_t;


uint8_t FCP_CalcCRC( FCP_Frame_t * pFrame )
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

inline int32_t fsp_frame_parse(uint8_t *Buffer, uint32_t *size)
{
  uint8_t crc;
  uint32_t lsize = *size;
  FCP_Frame_t * pFrame = (FCP_Frame_t*) Buffer;

  if (lsize < PAYLOAD_IDX)
  {
    return LISTENER_STATUS_UNDERFLOW;
  }
  else
  {

    if ((pFrame->Code != FCP_CODE_ACK) && (pFrame->Code != FCP_CODE_NACK))
      return LISTENER_STATUS_INVALID_CODE;


    if (lsize < pFrame->Size + CRC_OFFSET)
      return LISTENER_STATUS_UNDERFLOW;

    crc = pFrame->Buffer[pFrame->Size];

    if (crc != FCP_CalcCRC(pFrame))
    {
      return LISTENER_STATUS_CRC_ERROR;
    }

    // TODO something with the response
    // payload = bytearray(incomming[STM.PAYLOAD_IDX:payloadLen+STM.PAYLOAD_IDX])
    if( (pFrame->Size + CRC_OFFSET + 1) < lsize)
      Buffer = &Buffer[pFrame->Size + CRC_OFFSET + 1];
    else
      Buffer = NULL;

    *size -= pFrame->Size + CRC_OFFSET + 1;

    return LISTENER_STATUS_OK;
  }
  return 0;
}

int32_t fcp_receive( void * pHandle, uint8_t * buffer, uint32_t * size )
{
    uint32_t lsize = *size, nsize;
    uint8_t * pBuffer = buffer;
    int32_t ret = LISTENER_STATUS_OK;
    while( pBuffer && (lsize > 0) )
    {
        nsize = lsize;
        ret = fsp_frame_parse(pBuffer, &lsize);
        if(ret)
        {
          //TODO Diag
        }

        if( lsize == nsize )
          break;
    }
    buffer = pBuffer;
    *size = lsize;
    return ret;
}

