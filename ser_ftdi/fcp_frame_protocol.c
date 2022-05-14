

#include "fcp_frame_protocol.h"
#include <stddef.h>


/** @brief Size of the Header of an FCP frame */
#define FCP_HEADER_SIZE       2
/** @brief Maximum size of the payload of an FCP frame */
#define FCP_MAX_PAYLOAD_SIZE  128
/** @brief Size of the Control Sum (actually not a CRC) of an FCP frame */
#define FCP_CRC_SIZE          1
/** @brief Maximum size of an FCP frame, all inclusive */
#define FCP_MAX_FRAME_SIZE    (FCP_HEADER_SIZE + FCP_MAX_PAYLOAD_SIZE + FCP_CRC_SIZE)

#define KMC_TX_FRAME_SIZE 3

#define FCP_CODE_ACK  0xf0
#define FCP_CODE_NACK 0xff

#define FCP_STATUS_TRANSFER_ONGOING  0x01
#define FCP_STATUS_WAITING_TRANSFER  0x02
#define FCP_STATUS_INVALID_PARAMETER 0x03
#define FCP_STATUS_TIME_OUT          0x04
#define FCP_STATUS_INVALID_FRAME     0x05


uint8_t * fsp_frame_parse(uint8_t *Buffer, uint32_t * size);


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


uint8_t * fsp_frame_parse(uint8_t *Buffer, uint32_t * size)
{

  return 0;
}



int32_t fcp_receive( void * pHandle, uint8_t * buffer, uint32_t * size )
{
    uint32_t lsize = *size, nsize;
    uint8_t * pBuffer = buffer;
    while( pBuffer && (lsize > 0) )
    {
        nsize = lsize;
        pBuffer = fsp_frame_parse(pBuffer, &lsize);

        if( lsize == nsize )
          break;
    }
    if ( (buffer == pBuffer) || (*size = lsize) )
      return 1;
    buffer = pBuffer;
    *size = lsize;
    return 0;
}

