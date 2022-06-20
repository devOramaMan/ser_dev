

#ifndef SOCKET_COMMON
#define SOCKET_COMMON

typedef struct Topic_Type
{
  uint8_t code; 
  void* pMsgHandler;
}Topic_Type_t;

void init_socket_receiver(const char * connectstr);
void init_socket_sender(const char * connectstr);
void close_sockets(void);

int32_t sendData(uint8_t * buffer, uint32_t size);

void register_topics(Topic_Type_t * topics, uint32_t size );

#endif
