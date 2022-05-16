


#ifndef KMC_REC_PROTOCOL
#define KMC_REC_PROTOCOL

#include <stdint.h>

int32_t kmc_register_code(uint8_t val);

int32_t kmc_receive( void * pHandle, uint8_t *Buffer, uint32_t * Size );

#endif /* KMC_REC_PROTOCOL.h */
