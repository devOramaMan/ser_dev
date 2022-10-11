

#include "diag_frame_protocol.h"
#include <stddef.h>
#include "protocol_common.h"
#include "diagnostics_util.h"
#include <stdbool.h>
#include <stdio.h>



/* Defines ----------------------------------------------------------------*/

/** @brief Size of the Header of an Diagnostic frame */
#define DIAG_FRAME_HEADER_SIZE       4


#define DIAG_MAX_PRINT_SIZE 122


static char err_chr[] =	"ERROR";
static char warn_chr[] = "WARNING";
static char info_chr[] = "INFO";
static char deb_chr[] = "DEBUG";
static char rx_chr[] = "RXMSG";
static char tx_chr[] = "TXMSG";
static char asci_chr[] = "ASCII";
static char unk_chr[] =	"UNKNOWN";


char * diagType(uint8_t type)
{
  switch(type)
  {
	case DIAG_ERROR:
  return err_chr;
	case DIAG_WARNING:
  return warn_chr;
	case DIAG_INFO:
  return info_chr;
	case DIAG_DEBUG:
  return deb_chr;
	case DIAG_RXMSG:
  return rx_chr;
	case DIAG_TXMSG:
  return tx_chr;
	case DIAG_ASCII:
  return asci_chr;
  case DIAG_UNKNOWN:
  default:
  return unk_chr;
  break;
  }
  return unk_chr;
}

/**
 * @brief KMC Transmitter frame Header (base object)
 */

typedef struct Diag_Frame
{
  uint8_t code; /**< Identifier of the Frame. States the nature of the Frame. */
  uint8_t len;  /**< Size of the Payload of the frame in bytes. */
  uint8_t type;  /**< diagnostics type */
  uint8_t devid; /**< device identifier*/
  uint32_t timestamp; /**< Runtime clk*/
  char print[DIAG_MAX_PRINT_SIZE];
  uint8_t _CRC;
}Diag_Frame_t;



/* Local func -------------------------------------------------------------*/
uint8_t diag_frame_calc_CRC( Diag_Frame_t * pFrame );
int32_t diag_frame_parse(KMC_Diag_Handle_t * pHandle, uint8_t **Buffer, uint32_t * size);
void diag_print(char * print, uint16_t size);

/* Local var --------------------------------------------------------------*/




/**
  * @brief Calculate UINT8 CRC.
  * @details
  * @retval uint8_t, CRC result
  * @param[in] Diag_Frame_t, the input data for crc caculation
  */
inline uint8_t diag_frame_calc_CRC( Diag_Frame_t * pFrame )
{
  uint8_t nCRC = 0;
  uint16_t nSum = 0;
  uint8_t i;
  uint8_t size = pFrame->len;
  uint8_t * pBuffer = (uint8_t*)&pFrame->type;   
  for ( i = 0; i < size; i++ )
    nSum += *pBuffer++;

  nCRC = (uint8_t)(nSum & 0xFF) ; // Low Byte of nSum
  nCRC += (uint8_t) (nSum >> 8) ; // High Byte of nSum

  return nCRC;
}

inline void diag_print(char * print, uint16_t size)
{
  uint16_t i;
  char * pPrint = print;
  for(i = 0; i < (size); i++)
  {
    if( *pPrint != '\n' && *pPrint != '\r')
      printf("%c",*pPrint);
    pPrint++;
  }
  printf("\n");
}


inline int32_t diag_frame_parse(KMC_Diag_Handle_t * pHandle, uint8_t **Buffer, uint32_t * size)
{ 
  uint8_t  crc;
  uint32_t lsize = *size;
  uint8_t *pBuffer = *Buffer;
  Diag_Frame_t * pFrame = (Diag_Frame_t*)pBuffer;
  KMC_Diag_Dev_t * pDiagDev = pHandle->DiagDevs;
  uint16_t i;
  uint16_t len = 0;
  bool hasDev = false;
  char * diagTypeStr;

  if (lsize < 2)
  {
    return PROTOCOL_STATUS_UNDERFLOW;
  }

  if (pFrame->code != DIAG_FRAME_CODE)
  {
    return PROTOCOL_STATUS_INVALID_CODE;
  }


  if(lsize < (pFrame->len + 2))
    return PROTOCOL_STATUS_UNDERFLOW;
  
  
  crc = pBuffer[pFrame->len + 2];

  if( crc != diag_frame_calc_CRC(pFrame) )
  {
    return PROTOCOL_CODE_CRC_ERROR;
  }
  if (pFrame->len > 6 )
  {
    len =  pFrame->len - 6;
  }

  DiagMsg(DIAG_DEBUG, "pBuffer %p, len %d, pFrame->len %d", pBuffer, lsize, pFrame->len);

  for( i = 0; i < pHandle->DevSize; i++)
  {
    if(pDiagDev->id == pFrame->devid)
    {
      hasDev = true;
      if(pFrame->type != DIAG_ASCII)
      {
        diagTypeStr = diagType(pFrame->type);
        printf("%s: %.12d %.20s ", pDiagDev->name, pFrame->timestamp, diagTypeStr);
      }
      else
      {
        printf("%s: ", pDiagDev->name);
      }
      diag_print(pFrame->print, len);
    }
    pDiagDev++;
  }

  if(!hasDev)
  {
    if(pFrame->type != DIAG_ASCII)
    {
      printf("Unknown Dev:%.12d %.20s ", pFrame->timestamp, diagTypeStr);
    }
    else
    {
      printf("Unknown Dev: %.12d ", pFrame->timestamp);
    }
    diag_print(pFrame->print, len);
  }


  // TODO something with the response
  if( (pFrame->len + 2 + 1 ) < lsize)
  {
    *Buffer = &pBuffer[pFrame->len + 2 + 1];
    *size = (lsize - (pFrame->len + 2 + 1));
  }
  else
  {
    *size = 0;
  }

  return PROTOCOL_STATUS_OK;
}



int32_t diag_receive( Protocol_t * pHandle, uint8_t ** buffer, uint32_t * size )
{
  int32_t  ret = PROTOCOL_STATUS_OK;
  if ( *buffer && (*size > 0) )
  {
    ret = diag_frame_parse((KMC_Diag_Handle_t*)pHandle, buffer, size);
    if (!ret)
    {
      ret = PROTOCOL_STATUS_OK;
      pHandle->Diag.MSG_OK++;
    }
    else
    {
      if(ret == PROTOCOL_CODE_CRC_ERROR)
      {
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

