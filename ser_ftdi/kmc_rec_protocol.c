

#include "kmc_rec_protocol.h"
#include <stddef.h>
#include "util_common.h"
#include <stdbool.h>


/* Defines ----------------------------------------------------------------*/

/** @brief Index of the kmc frame payload lenght*/
#define KMC_REC_PAYLOAD_LEN_IDX   1
/** @brief Index of the msg counter*/
#define KMC_REC_CNT_IDX           2
/** @brief Index of the payload */
#define KMC_REC_PAYLOAD_IDX       3
/** @brief Offset of the CRC from payload length*/
#define KMC_REC_CRC_OFFSET        2
/** @brief Size of the frame size (total msg size = payload lenght + this define)*/
#define KMC_TX_FRAME_SIZE         3

#define KMC_REC_MAX_CODES         20

/**
 * @brief KMC Transmitter frame Header (base object)
 */
typedef struct KMC_Transmit_Frame_s
{
	uint8_t Code;         /**< Identifier of the Frame. States the nature of the Frame. */
	const uint8_t Size;   /**< Size of the Payload of the frame in bytes. */
  uint8_t cnt;          /**< cnt to verify all is received. */
}KMC_Rec_Frame_t;


/* Local func -------------------------------------------------------------*/
int16_t kmc_frame_has_code( uint8_t code );
uint8_t kmc_frame_calc_CRC( KMC_Rec_Frame_t * pFrame );
int32_t kmc_frame_parse(uint8_t *Buffer, uint32_t * size);


/* Local var --------------------------------------------------------------*/
volatile uint8_t rec_codes[KMC_REC_MAX_CODES] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
volatile uint8_t rec_code_size = 0;


/**
 * @brief This structure contains and formats a Frame Communication Protocol's frame
 */
int16_t kmc_frame_has_code( uint8_t code )
{
  int32_t i = rec_code_size - 1; 
  while(i >= 0)
  {
    if( (rec_codes[i] != 0) && (rec_codes[i] == code) )
      break;
    i--;
  }
  return i;
}


/**
  * @brief Calculate UINT8 CRC.
  * @details
  * @retval uint8_t, CRC result
  * @param[in] KMC_Rec_Frame_t, the input data for crc caculation
  */
inline uint8_t kmc_frame_calc_CRC( KMC_Rec_Frame_t * pFrame )
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


inline int32_t kmc_frame_parse(uint8_t *Buffer, uint32_t * size)
{ 
  uint8_t  crc;
  uint32_t lsize = *size;
  KMC_Rec_Frame_t * pFrame = (KMC_Rec_Frame_t*)Buffer;

  if (lsize < KMC_REC_PAYLOAD_IDX)
  {
    return LISTENER_STATUS_UNDERFLOW;
  }
  else
  {
    if (kmc_frame_has_code(pFrame->Code)<0)
    {
      return LISTENER_STATUS_INVALID_CODE;
    }

    if(lsize < (pFrame->Size + KMC_REC_PAYLOAD_IDX))
      return LISTENER_STATUS_UNDERFLOW;
    
    //TODO Handle cnt
    // pFrame->cnt;
    crc = Buffer[pFrame->Size + KMC_REC_PAYLOAD_IDX];

    if( crc != kmc_frame_calc_CRC(pFrame) )
    {
      return LISTENER_STATUS_CRC_ERROR;
    }

    // TODO something with the response
    if( (pFrame->Size + KMC_REC_PAYLOAD_IDX + 1 ) < lsize)
      Buffer = &Buffer[pFrame->Size + KMC_REC_PAYLOAD_IDX + 1];
    else
      Buffer = NULL;
      
    *size -= pFrame->Size + KMC_REC_PAYLOAD_IDX + 1;

    return LISTENER_STATUS_OK;
  }


  

  // if kmc_id not in KMC_TX_FRAME_ID:
  //     return self.KmcParsePass, incomming, incomming, size #not fcp msg
  //     #raise KmcParseError("Unknown Id: %d, %d - %s" % (kmc_id, size, str(errmsg)))
  
  // payloadLen = incomming[KMC_REC_PAYLOADLEN_IDX]
  // crc_idx = payloadLen +  KMC_REC_PAYLOAD_IDX
  
  // if(size <= (crc_idx)):
  //     return kmcPartBuffer.setLast, incomming, bytes(), 0
  //     #raise KmcParseError("Shortlen msg: %d, %d - %s" % (payloadLen, size, errmsg))

  // cnt = incomming[KMC_REC_CNT_IDX]
  
  // crc = incomming[crc_idx]
  
  // crc_calc = STM._calcCrc(incomming[:payloadLen+KMC_REC_PAYLOAD_IDX])
  
  // #if crc not ok then not a tx msg something else
  // if crc != crc_calc:
  //     return self.KmcParseErr, [incomming, str("crcErr: %d, %d, %d - %s" % (crc, crc_calc, size, str(errmsg)))], bytes(), size
  //     #raise KmcParseError("crcErr: %d, %d, %d - %s" % (crc, crc_calc, size, str(errmsg)))
  
  // payload = incomming[KMC_REC_PAYLOAD_IDX:crc_idx]
  
  // if kmc_id in FRAME_CODE_FUNC_PTR:
  //     return FRAME_CODE_FUNC_PTR[kmc_id], payload, incomming[KMC_REC_PAYLOADLEN_IDX+payloadLen+KMC_REC_PAYLOAD_IDX:], cnt

  // return self.KmcParseErr, [incomming, str("Missig fptr: %d- %s" % ( size, errmsg))], bytes(), size
  // #raise KmcParseError("Missig fptr: %d- %s" % ( size, errmsg))
  //   return 0;
}

int32_t kmc_register_code(uint8_t val)
{
  uint32_t i = 0;
  if ( MAX_PRINT_SIZE >= rec_code_size)
    DiagMsg(DIAG_ERROR, "code is full");
  while(i < rec_code_size)
  {
    if(rec_codes[i] == val)
      break;
    i++;
  }
  if ( i < rec_code_size)
  {
    DiagMsg(DIAG_ERROR, "Register code exist (%d)", val);
    return 1;
  }
  rec_codes[i] = val;
  return 0;
}

int32_t kmc_unregister_code(uint8_t val)
{
  uint32_t i = 0;
  while( i < rec_code_size )
  {
    if(rec_codes[i] == val)
    {
      rec_codes[i] = 0;
      break;
    }
    i++;
  }

  if ( i < rec_code_size )
  {
    DiagMsg(DIAG_ERROR, "Register code exist (%d)", val);
    return 1;
  }

  rec_codes[i] = val;
  return 0;
}

int32_t kmc_receive( void * pHandle, uint8_t * buffer, uint32_t * size )
{
  uint32_t lsize = *size, nsize;
  uint8_t *pBuffer = buffer;
  int32_t  ret = LISTENER_STATUS_OK;
  while (pBuffer && (lsize > 0))
  {
    nsize = lsize;
    ret = kmc_frame_parse(pBuffer, &lsize);

    if (lsize == nsize)
      break;
  }
  buffer = pBuffer;
  *size = lsize;
  return ret;
}

