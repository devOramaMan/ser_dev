

#include "kmc_rec_protocol.h"
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


uint8_t * kmc_frame_parse(uint8_t * Buffer, uint32_t * size);

/**
 * @brief STM32 MC Interface Handle
 */




/**
 * @brief KMC Transmitter frame Header (base object)
 */
typedef struct KMC_Transmit_Frame_s
{
	uint8_t Code;         /**< Identifier of the Frame. States the nature of the Frame. */
	const uint8_t Size;   /**< Size of the Payload of the frame in bytes. */
    uint8_t cnt;          /**< cnt to verify all is received. */
}KMC_Transmit_Frame_t;

/**
 * @brief This structure contains and formats a Frame Communication Protocol's frame
 */



/**
  * @brief Calculate UINT8 CRC.
  * @details
  * @retval uint8_t, CRC result
  * @param[in] KMC_Transmit_Frame_t, the input data for crc caculation
  */
inline uint8_t kmc_frame_CalcCRC( KMC_Transmit_Frame_t * pFrame )
{
  uint8_t nCRC = 0;
  uint16_t nSum = 0;
  uint8_t i;
  uint8_t size = pFrame->Size + KMC_TX_FRAME_SIZE;
  uint8_t * pBuffer = (uint8_t*)pFrame;   
  for ( i = 0; i < size; i++ )
    nSum += *pBuffer++;

  nCRC = (uint8_t)(nSum & 0xFF) ; // Low Byte of nSum
  nCRC += (uint8_t) (nSum >> 8) ; // High Byte of nSum

  return nCRC;
}


inline uint8_t * kmc_frame_parse(uint8_t * Buffer, uint32_t * size)
{
    
    return 0;
}



int32_t kmc_receive( void * pHandle, uint8_t * buffer, uint32_t * size )
{
   uint32_t lsize = *size, nsize;
    uint8_t * pBuffer = buffer;
    while( pBuffer && (lsize > 0) )
    {
        nsize = lsize;
        pBuffer = kmc_frame_parse(pBuffer, &lsize);

        if( lsize == nsize )
          break;
    }
    if ( (buffer == pBuffer) || (*size = lsize) )
      return 1;
    buffer = pBuffer;
    *size = lsize;
    return 0;
    return 0;
}

