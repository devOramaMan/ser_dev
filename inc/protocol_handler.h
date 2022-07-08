/*-----------------------------------------------------------------------------
 * Name:    protocol_handler.h
 * Purpose: Route the incomming buffer
 *-----------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/


#ifndef PROTOCOL_HANDLER
#define PROTOCOL_HANDLER

#include <stdint.h>
#include <protocol_common.h>

typedef struct Protocol_Handler
{
  Protocol_Diag_t Diag;
  Protocol_Handle_t * ProtocolList;
  uint16_t size;
} Protocol_Handler_t;

int32_t receive_data(void * pHandler, uint8_t ** buffer, uint32_t size );

#endif
