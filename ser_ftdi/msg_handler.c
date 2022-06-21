
#include "msg_handler.h"
#include "diagnostics_util.h"
#include "fcp_frame_protocol.h"
#include "atomic_queue.h"
#include <stdio.h>
#include <config.h>

#define FCP_SINGLE_PAYLOAD 8

uint32_t msg_fcp_single(void* pCallback, uint8_t *buffer)
{
  int32_t size;
  uint8_t code =  *buffer;
  Msg_Fcp_Single_t *in_msg = (Msg_Fcp_Single_t *)&buffer[1];
  FCP_Frame_t fcp_msg;
  Atomic_Queue_Msg_t out_msg;
  int32_t crc_idx;
  uint8_t * value;
  uint32_t ret = 0, i ;

  DiagMsg(DIAG_DEBUG, "msg_fcp_single (size: %d, id %d)", in_msg->size, in_msg->transaction_id);

  size = in_msg->size - FCP_SINGLE_PAYLOAD;
  if (size < 0)
  {
    DiagMsg(DIAG_ERROR, "msg_fcp_single Size %d", size);
    return 1;
  }
  //Value address
  value = (&buffer[FCP_SINGLE_PAYLOAD] + 1);
  crc_idx = size + 1;

  fcp_msg.Code = (in_msg->nodeid << 5);
  fcp_msg.Code |= in_msg->frameid  & ~(0xE0);
  fcp_msg.Buffer[0] = (uint8_t)in_msg->startReg;
  fcp_msg.Size = crc_idx;
  for (i = 0; i < size; i++)
    fcp_msg.Buffer[i + 1] = value[i];
  fcp_msg.Buffer[crc_idx] = fcp_calc_crc(&fcp_msg);

#if (DEBUG_FCP_MSG == 1)
  uint8_t * ptr = (uint8_t*)&fcp_msg;
  for (i = 0; i < fcp_msg.Size + 3; i++)
  {
    printf("0x%02X ", *ptr);
    ptr++;
  }
  printf("\n");
#endif

  out_msg.callback = pCallback;
  out_msg.code = code;
  out_msg.id = in_msg->transaction_id;
  out_msg.buffer = (uint8_t*)&fcp_msg;
  out_msg.size = (fcp_msg.Size + 3);
  ret = append_send_msg(&out_msg);

  return ret;
}