


#ifndef DIAG_FRAME_PROTOCOL
#define DIAG_FRAME_PROTOCOL

#include <stdint.h>
#include <protocol_common.h>

#define DIAG_FRAME_CODE         0xCC

typedef struct KMC_Diag_Dev_Type
{
  uint8_t id;
  char * name;
}KMC_Diag_Dev_t;

typedef struct KMC_Diag_Handle
{
  Protocol_t _super;
  KMC_Diag_Dev_t * DiagDevs;
  uint16_t DevSize;
}KMC_Diag_Handle_t;

int32_t diag_receive( Protocol_t * pHandle, uint8_t ** buffer, uint32_t * size );

#endif /* DIAG_FRAME_PROTOCOL.h */
