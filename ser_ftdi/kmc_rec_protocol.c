

#include "kmc_rec_protocol.h"
#include <stddef.h>
#include "protocol_common.h"
#include "diagnostics_util.h"
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
/** @brief Size of the Header of an FCP frame */
#define KMC_REX_HEADER_SIZE       3

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


typedef struct KMC_Rec_Handle
{
  Protocol_t _super;
  uint8_t rec_codes[KMC_REC_MAX_CODES];
  uint8_t rec_code_size;
}KMC_Rec_Handle_t;


/* Local func -------------------------------------------------------------*/
int16_t kmc_frame_has_code( uint8_t code );
uint8_t kmc_frame_calc_CRC( KMC_Rec_Frame_t * pFrame );
int32_t kmc_frame_parse(uint8_t *Buffer, uint32_t * size);
int32_t kmc_receive( Protocol_t * pHandle, uint8_t *Buffer, uint32_t * Size );


/* Local var --------------------------------------------------------------*/
KMC_Rec_Handle_t kmc_Rec_Handle = 
{
  .rec_codes = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  .rec_code_size = 0
};


/**
 * @brief This structure contains and formats a Frame Communication Protocol's frame
 */
int16_t kmc_frame_has_code( uint8_t code )
{
  int32_t i = kmc_Rec_Handle.rec_code_size - 1; 
  while(i >= 0)
  {
    if( (kmc_Rec_Handle.rec_codes[i] != 0) && (kmc_Rec_Handle.rec_codes[i] == code) )
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

  if (lsize < KMC_TX_FRAME_SIZE)
  {
    return PROTOCOL_STATUS_UNDERFLOW;
  }
  else
  {
    if (kmc_frame_has_code(pFrame->Code)<0)
    {
      return PROTOCOL_STATUS_INVALID_CODE;
    }

    if(lsize < (pFrame->Size + KMC_TX_FRAME_SIZE))
      return PROTOCOL_STATUS_UNDERFLOW;
    
    //TODO Handle cnt
    // pFrame->cnt;
    crc = Buffer[pFrame->Size + KMC_TX_FRAME_SIZE];

    if( crc != kmc_frame_calc_CRC(pFrame) )
    {
      return PROTOCOL_STATUS_CRC_ERROR;
    }

    // TODO something with the response
    if( (pFrame->Size + KMC_TX_FRAME_SIZE + 1 ) < lsize)
      Buffer = &Buffer[pFrame->Size + KMC_TX_FRAME_SIZE + 1];
    else
      Buffer = NULL;
      
    *size -= pFrame->Size + KMC_TX_FRAME_SIZE + 1;

    return PROTOCOL_STATUS_OK;
  }
}

int32_t kmc_register_code(uint8_t val)
{
  uint32_t i = 0;
  if ( KMC_REC_MAX_CODES >= kmc_Rec_Handle.rec_code_size)
    DiagMsg(DIAG_ERROR, "Code is full");
  while(i < kmc_Rec_Handle.rec_code_size)
  {
    if(kmc_Rec_Handle.rec_codes[i] == val)
      break;
    i++;
  }
  if ( i < kmc_Rec_Handle.rec_code_size)
  {
    DiagMsg(DIAG_ERROR, "Register code exist (%d)", val);
    return 1;
  }
  kmc_Rec_Handle.rec_codes[i] = val;
  return 0;
}

int32_t kmc_unregister_code(uint8_t val)
{
  uint32_t i = 0;
  while( i < kmc_Rec_Handle.rec_code_size )
  {
    if(kmc_Rec_Handle.rec_codes[i] == val)
    {
      kmc_Rec_Handle.rec_codes[i] = 0;
      break;
    }
    i++;
  }

  if ( i < kmc_Rec_Handle.rec_code_size )
  {
    DiagMsg(DIAG_ERROR, "Register code exist (%d)", val);
    return 1;
  }

  kmc_Rec_Handle.rec_codes[i] = val;
  return 0;
}

int32_t kmc_receive( Protocol_t * pHandle, uint8_t * buffer, uint32_t * size )
{
  uint32_t lsize = *size, nsize;
  uint8_t *pBuffer = buffer;
  int32_t  ret = PROTOCOL_STATUS_OK;
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

