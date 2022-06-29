

#ifndef _FILE_HANDLER_H__
#define _FILE_HANDLER_H__

#include <stdint.h>
#include "func_common.h"

typedef struct File_Type
{
  uint8_t code;
  uint8_t nodeId;
  uint8_t fileNum;
  uint8_t type;
  uint32_t size;
  uint32_t delay;
  uint32_t transaction_id;
} File_Type_t;
  


void start_file_thread(void);
void stop_file_thread(void);

int32_t append_read_file( void * callback, File_Type_t * file );
int32_t append_write_file( void * callback, File_Type_t * file );

#endif

