
#ifndef FUNC_COMMON
#define FUNC_COMMON

#include <stdint.h>

typedef void (*PROC_SIGNAL)( void );

typedef int32_t (*RX_TX_FUNC)( uint8_t * buffer, uint32_t size );

typedef int32_t (*RX_TX_CODE_FUNC)( uint8_t * buffer, uint32_t size , uint32_t code );

typedef int32_t (*RET_RX_TX_FUNC)( uint8_t * buffer, uint32_t size, uint32_t * ret_size );

#endif
