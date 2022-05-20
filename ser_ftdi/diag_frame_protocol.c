

#include "diag_frame_protocol.h"
#include <stddef.h>
#include "protocol_common.h"
#include "diagnostics_util.h"
#include <stdbool.h>



/* Defines ----------------------------------------------------------------*/

/** @brief Size of the Header of an Diagnostic frame */
#define DIAG_FRAME_HEADER_SIZE       4

#define DIAG_FRAME_CODE         0xDF

/**
 * @brief KMC Transmitter frame Header (base object)
 */
typedef struct Diag_Frame
{
	uint8_t Code;         /**< Identifier of the Frame. States the nature of the Frame. */
	uint8_t Size;   /**< Size of the Payload of the frame in bytes. */
  uint8_t Time;          /**< cnt to verify all is received. */
  uint8_t Type;
}Diag_Frame_t;


typedef struct KMC_Rec_Handle
{
  Protocol_t _super;
}KMC_Rec_Handle_t;


/* Local func -------------------------------------------------------------*/
uint8_t diag_frame_calc_CRC( Diag_Frame_t * pFrame );
int32_t diag_frame_parse(uint8_t *Buffer, uint32_t * size);
int32_t diag_receive( Protocol_t * pHandle, uint8_t * buffer, uint32_t * size );

/* Local var --------------------------------------------------------------*/




/**
  * @brief Calculate UINT8 CRC.
  * @details
  * @retval uint8_t, CRC result
  * @param[in] KMC_Rec_Frame_t, the input data for crc caculation
  */
inline uint8_t kmc_frame_calc_CRC( Diag_Frame_t * pFrame )
{
  uint8_t nCRC = 0;
  uint16_t nSum = 0;
  uint8_t i;
  uint8_t size = pFrame->Size + DIAG_FRAME_HEADER_SIZE;
  uint8_t * pBuffer = (uint8_t*)pFrame;   
  for ( i = 0; i < size; i++ )
    nSum += *pBuffer++;

  nCRC = (uint8_t)(nSum & 0xFF) ; // Low Byte of nSum
  nCRC += (uint8_t) (nSum >> 8) ; // High Byte of nSum

  return nCRC;
}


inline int32_t diag_frame_parse(uint8_t *Buffer, uint32_t * size)
{ 
  uint8_t  crc;
  uint32_t lsize = *size;
  Diag_Frame_t * pFrame = (Diag_Frame_t*)Buffer;

  if (lsize < DIAG_FRAME_HEADER_SIZE)
  {
    return PROTOCOL_STATUS_UNDERFLOW;
  }
  else
  {
    if (pFrame->Code != DIAG_FRAME_CODE)
    {
      return PROTOCOL_STATUS_INVALID_CODE;
    }

    if(lsize < (pFrame->Size + DIAG_FRAME_HEADER_SIZE))
      return PROTOCOL_STATUS_UNDERFLOW;
    
    //TODO Handle cnt
    // pFrame->cnt;
    crc = Buffer[pFrame->Size + DIAG_FRAME_HEADER_SIZE];

    if( crc != kmc_frame_calc_CRC(pFrame) )
    {
      return PROTOCOL_STATUS_CRC_ERROR;
    }

    // TODO something with the response
    if( (pFrame->Size + DIAG_FRAME_HEADER_SIZE + 1 ) < lsize)
      Buffer = &Buffer[pFrame->Size + DIAG_FRAME_HEADER_SIZE + 1];
    else
      Buffer = NULL;
      
    *size -= pFrame->Size + DIAG_FRAME_HEADER_SIZE + 1;

    return PROTOCOL_STATUS_OK;
  }
}



int32_t diag_receive( Protocol_t * pHandle, uint8_t * buffer, uint32_t * size )
{
  uint32_t lsize = *size, nsize;
  uint8_t *pBuffer = buffer;
  int32_t  ret = PROTOCOL_STATUS_OK;
  while (pBuffer && (lsize > 0))
  {
    nsize = lsize;
    ret = diag_frame_parse(pBuffer, &lsize);

    if (lsize == nsize)
      break;
  }
  buffer = pBuffer;
  *size = lsize;
  return ret;
}

