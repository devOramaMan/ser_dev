/*-----------------------------------------------------------------------------
 * Name:    protocol_handler.c
 * Purpose: route the incomming buffer
 *-----------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/


/* Private includes ----------------------------------------------------------*/
#include "protocol_handler.h"
#include "protocol_common.h"
#include "diagnostics_util.h"
#include "util_common.h"

//putchar
#include <stdio.h>

/* Private functions ---------------------------------------------------------*/
bool hasCode(Protocol_t *prot, uint8_t code);
int32_t handleAscii(uint8_t *buffer, uint32_t *size);

bool hasCode(Protocol_t *prot, uint8_t code)
{
  uint32_t i; 
  bool ret = false;

  if (!prot)
    return ret;

  for (i = 0; i < prot->Size; i++)
  {
    if (prot->Code[i] == code)
    {
      ret = true;
      break;
    }
  }

  return ret;
}

__weak int32_t handleAscii(uint8_t *buffer, uint32_t *size)
{
  //TODO take sub array of array of ascii
  if (!checkAscii(buffer, *size))
  {
    if (diag_get_verbose() & DIAG_ASCII)
    {
      while (*size > 0)
      {
        putchar(*buffer++);
        *size -= 1;
      }
    }
  }
  else
    return 1;
  return 0;
}

int32_t receive_data(void *pHandler, uint8_t **buffer, uint32_t size)
{
  int32_t ret = PROTOCOL_STATUS_OK;
  uint32_t i;
  PROTOCOL_CALLBACK callback;
  Protocol_Handler_t *plHandler = (Protocol_Handler_t *)pHandler;
  uint8_t * pBuffer = *buffer;
  uint32_t slen = size;

  while (slen > 0)
  {
    callback = NULL;
    for (i = 0; i < plHandler->size; i++)
    {
      if ( hasCode(plHandler->ProtocolList[i].pProtocol, *pBuffer) )
      {
        callback = plHandler->ProtocolList[i].pCallback;
        break;
      }
    }
    
    if (callback)
    {

      ret = callback(plHandler->ProtocolList[i].pProtocol, &pBuffer, &slen);
      if (!ret)
      {
        plHandler->Diag.MSG_OK++;
      }
      else
      {
        if (ret == PROTOCOL_STATUS_UNDERFLOW)
        {
          if (pBuffer > *buffer)
          {
            for (i = 0; i < slen; i++)
              (*buffer)[i] = pBuffer[i];
          }
          *buffer = (*buffer) + slen;
          plHandler->Diag.UNDERFLOW++;
          DiagMsg(DIAG_RXMSG, "Underflow (pBuffer %p, buffer %p ,code %d, len %d)", pBuffer, *buffer, *pBuffer, slen);
          slen = 0;
        }
        else
        {
          if (handleAscii(pBuffer, &slen))
          {
            ret = PROTOCOL_STATUS_INVALID_CODE;
            DiagMsg(DIAG_RXMSG, "Invalid msg");
            if (ret == PROTOCOL_CODE_CRC_ERROR)
              plHandler->Diag.CRC_ERROR++;
            else if (ret == PROTOCOL_STATUS_INVALID_CODE)
              plHandler->Diag.UNKNOWN_MSG++;

            plHandler->Diag.RX_ERROR++;
          }
        }
      }
    }
    else
    {
      if (handleAscii(pBuffer, &slen))
      {
        ret = PROTOCOL_STATUS_INVALID_CODE;
        plHandler->Diag.UNKNOWN_MSG++;
        plHandler->Diag.RX_ERROR++;
      }
    }

    if (ret)
    {
      if (slen > 0 && pBuffer)
      {
        pBuffer++;
        slen--;
      }
    }
  }

  return ret;
}
