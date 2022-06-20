

#ifndef ATOMIC_QUEUE
#define ATOMIC_QUEUE
#include <stdint.h>
#include "func_common.h"

typedef struct Atomic_Queue_Msg
{
    void * callback;
    uint8_t code;
    uint32_t id;
    uint8_t * buffer;
    uint32_t size;
}Atomic_Queue_Msg_t;
  


void start_queue_thread(RET_RX_TX_FUNC sendFunc);
void stop_queue_thread(void);

int32_t signal_received(uint8_t * buffer, uint32_t size, uint32_t code);

int32_t append_send_queue( uint8_t * buffer, uint32_t size );

int32_t append_send_msg( const Atomic_Queue_Msg_t * msg );

#endif