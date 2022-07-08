
#ifndef MSG_HANDLER
#define MSG_HANDLER

#include <stdint.h>



typedef struct Msg_Fcp_Single
{
  uint8_t size;
  uint8_t nodeid;
  uint8_t frameid;
  uint8_t startReg;
  uint32_t transaction_id;
} Msg_Fcp_Single_t;




uint32_t msg_fcp_single(void * pCallback, uint8_t * buffer);
uint32_t msg_kmc_rec_subscribe(void* pCallback, uint8_t *buffer);


#endif