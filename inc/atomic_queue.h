

#ifndef ATOMIC_QUEUE
#define ATOMIC_QUEUE
#include <stdint.h>
#include "func_common.h"

void start_queue_thread(RET_RX_TX_FUNC sendFunc);
void stop_queue_thread(void);

void signal_received(void);

int32_t append_send_queue( uint8_t * buffer, uint32_t size);

#endif