/*-----------------------------------------------------------------------------
 * Name:    protocol_diag.c
 * Purpose: protocol diagnostic 
 *-----------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/

#include "protocol_diag.h"
#include "protocol_config.h"
#include <stdio.h>

void protocol_diagnostics_print(void)
{
  int32_t i;
  Protocol_Diag_List_t * pDiagList = protocol_diag_handle.diagList;
  Protocol_Diag_t * pDiag;
  for( i = 0; i < protocol_diag_handle.size; i++)
  {
    pDiag = (Protocol_Diag_t*)pDiagList->diag;
    printf("%-18.18s { Ok %d, RX Err %d, TX Err %d, CRC Err %d, UnderFlw %d, Unknown msg %d, Timeout %d  }\n"\
    , pDiagList->name \
    , pDiag->MSG_OK \
    , pDiag->RX_ERROR \
    , pDiag->TX_ERROR \
    , pDiag->CRC_ERROR \
    , pDiag->UNDERFLOW \
    , pDiag->UNKNOWN_MSG \
    , pDiag->TIMEOUT );
    pDiagList++;
  }
}

void protocol_diagnostics_clear(void)
{
    int32_t i;
  Protocol_Diag_List_t * pDiagList = protocol_diag_handle.diagList;
  Protocol_Diag_t * pDiag;
  for( i = 0; i < protocol_diag_handle.size; i++)
  {
    pDiag = (Protocol_Diag_t*)pDiagList->diag;
    pDiag->MSG_OK = 0;
    pDiag->RX_ERROR = 0;
    pDiag->TX_ERROR = 0;
    pDiag->CRC_ERROR = 0;
    pDiag->UNDERFLOW = 0;
    pDiag->UNKNOWN_MSG = 0;
    pDiag->TIMEOUT  = 0;
    pDiagList++;
  }
}

uint32_t protocol_diagnostics_dump(char * buffer, uint32_t size)
{
  int32_t lsize;
  uint32_t buf_size, ret;
  int32_t i;
  Protocol_Diag_List_t * pDiagList = protocol_diag_handle.diagList;
  Protocol_Diag_t * pDiag;

  buf_size = size;
  lsize = 0;
  ret = 0;
  for( i = 0; i < protocol_diag_handle.size && (buf_size > 0); i++)
  {
    pDiag = (Protocol_Diag_t*)pDiagList->diag;
    lsize = snprintf(&buffer[ret], buf_size, "%-18.18s { Ok %d, RX Err %d, TX Err %d, CRC Err %d, UnderFlw %d, Unknown msg %d, Timeout %d  }\n"\
    , pDiagList->name \
    , pDiag->MSG_OK \
    , pDiag->RX_ERROR \
    , pDiag->TX_ERROR \
    , pDiag->CRC_ERROR \
    , pDiag->UNDERFLOW \
    , pDiag->UNKNOWN_MSG \
    , pDiag->TIMEOUT );

    buf_size -= lsize;
    ret += lsize;
    pDiagList++;
  }
  return ret;
}
