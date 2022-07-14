

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <zmq.h>

#include "socket_com.h"
#include "diagnostics_util.h"
#include "protocol_common.h"
#include "config.h"


#define CODE_SIZE 1
#define TRANSACTION_SIZE 4



typedef struct Socket_Term
{
    const char *connectstr;
    pthread_t threadId;
    volatile bool stop;
    volatile bool running;
    void* context;
    void* publisher;
    Topic_Type_t * topics;
    uint32_t topic_size;
    int32_t timeout;

}Socket_Com_t;

Socket_Com_t socket_com =
{
  .stop = true,
  .running = false,
  .context = NULL,
  .publisher = NULL,
  .context = NULL,
  .topics = NULL,
  .topic_size = 0,
  .timeout = 1000
};

//static const uint32_t topic_size = sizeof(socket_com.topic);
void *socket_com_reciv(void *arg);

void init_socket_receiver(const char * connectstr)
{
  uint32_t ret;
  pthread_attr_t attr;
  struct sched_param sp;
  socket_com.connectstr = connectstr;
  if(socket_com.running)
  {
    DiagMsg(DIAG_ERROR, "Socket Receiver is already running");
    return;
  }

  ret = pthread_attr_init(&attr);
  if (ret)
  {
    DiagMsg(DIAG_ERROR, "socket receive thread attrib init failed (%d)", ret);
  }

  if (pthread_attr_getschedparam(&attr, &sp))
    DiagMsg(DIAG_ERROR, "socket receive thread attrib prio failed ");
  else
    DiagMsg(DIAG_DEBUG, "Socket Com thread priority %d", sp.sched_priority);

  if(!ret)
  {
    socket_com.stop = false;
    ret = pthread_create(&(socket_com.threadId), &attr, &socket_com_reciv, (void *)&socket_com);
  }

  if(ret)
    DiagMsg(DIAG_ERROR, "Failed to start socket receiver");
}

void init_socket_sender(const char * connectstr)
{
  
  if(socket_com.context)
  {
    DiagMsg(DIAG_WARNING, "Sender Already open");
    return;
  }
  
  socket_com.context = zmq_ctx_new ();

  socket_com.publisher = zmq_socket (socket_com.context, ZMQ_PUB);

  if(zmq_bind(socket_com.publisher, connectstr))
  {
    DiagMsg(DIAG_ERROR, "Receiver socket failed to connect %s", connectstr);
    socket_com.publisher = NULL;
  }
  else
    DiagMsg(DIAG_DEBUG, "Socket Sender started %s" , connectstr);
}



void close_sockets(void)
{
  int32_t cnt = 10;
  socket_com.stop = true;

  while(socket_com.running && cnt > 0)
  {
    sleep(1);
    cnt--;  
  }
  if(!cnt)  
    DiagMsg(DIAG_ERROR, "Failed to close socket com receiver");
  

  if(socket_com.publisher)
  {
    zmq_close (socket_com.publisher);
    socket_com.publisher = NULL;
  }

  if(socket_com.context)
  {
    zmq_ctx_destroy (socket_com.context);
    socket_com.context = NULL;
  }
}

void *socket_com_reciv(void *arg)
{
  uint8_t message[100];
  Socket_Com_t * pSCom = (Socket_Com_t *)arg;
  Topic_Type_t * topic;
  int32_t ret;
  uint32_t i;
  void * subscriber;
  void * context = zmq_ctx_new ();
  
  subscriber = zmq_socket (context, ZMQ_SUB);

  if(zmq_connect(subscriber, pSCom->connectstr))
  {
    DiagMsg(DIAG_ERROR, "Receiver socket failed to connect %s", pSCom->connectstr);
    return NULL;
  }

  if(zmq_setsockopt(subscriber,ZMQ_RCVTIMEO, &pSCom->timeout, sizeof(pSCom->timeout) ))
  {
    DiagMsg(DIAG_ERROR, "Receiver socket failed set timeout (%d)" , pSCom->timeout);
    return NULL;
  }

  if(zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, "", 0))
  {
    DiagMsg(DIAG_ERROR, "Receiver socket failed setup subscribe.");
    return NULL;
  }

  DiagMsg(DIAG_INFO, "Socket Receiver thread started (%s)", pSCom->connectstr);
  pSCom->running = true;
  while(!pSCom->stop)
  {
    ret = zmq_recv(subscriber, message, sizeof(message), 0);
    if(ret != EOF)
    {
      DiagMsg(DIAG_DEBUG,"msg received (code: %d)", message[0]);
      topic = pSCom->topics;
      for(i = 0; i < pSCom->topic_size; i++, topic++)
      {
        if(message[0] == topic->code)
          ((void(*)(void*, uint8_t *))topic->pMsgHandler)((void*)sendData, &message[0]);
      }
    }
    else
    {
      //Notify if error code is different than timout
      if(errno != EAGAIN )
        DiagMsg(DIAG_WARNING,"receive socket rec error (%d)", errno);
    }
  }

  zmq_close (subscriber);
  zmq_ctx_term(context);
  zmq_ctx_destroy(context);


  pSCom->running = false;
  DiagMsg(DIAG_INFO, "Socket Com Recieiver thread closed.");
  return NULL;
}

int32_t sendData(uint8_t * buffer, uint32_t size)
{

  if(!socket_com.publisher || socket_com.stop)
  {
    return EXIT_FAILURE;
  }
  #if (DEBUG_FCP_MSG == 1)
  int i;
  for (i = 0; i < size; i++)
  {
    printf("0x%02X ", buffer[i]);
  }
  printf("\n");
  #endif
  // code (u8) + " " + id (ui32) + payload (buffer)
  DiagMsg(DIAG_DEBUG, "Out msg size: %d",  size);
  const size_t rs = zmq_send(socket_com.publisher, buffer, size, 0);
  if (rs != size)
  {
    DiagMsg(DIAG_ERROR,"ZeroMQ error occurred during zmq_msg_init_size(): %s\n", zmq_strerror(errno));
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

void register_topics(Topic_Type_t * topics, uint32_t size )
{
  socket_com.topics = topics;
  socket_com.topic_size = size;
}
#define DEBUG_SENDER 0

int testSocket(void)
{
  #if (DEBUG_SENDER == 1)
  int i = 0;
  uint8_t buffer[20] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};
  // uint32_t size= 14;
  // uint32_t id = 23; 
  //uint8_t code = 10;
  //zmq_msg_t msg;
  //void * pmsg;
  
  init_socket_sender("tcp://127.0.0.1:5585");
  sleep(1);
  //init_socket_sender("ipc:///tmp/stream");
  void* data_socket = socket_com.publisher;

  while(i < 1)
  {
    sprintf((char*)buffer,"hello Workd %d\n" , i);
    if(zmq_send(data_socket,buffer, 14, 0) != 14)
    {
      DiagMsg(DIAG_ERROR, "failed to send");
      //break;
    }
    DiagMsg(DIAG_DEBUG, "Msg Sent");
    i++;
    sleep(1);
  }

  // code (u8) + " " + id (ui32) + payload (buffer)
  //const size_t msg_size = CODE_SIZE + 1 + TRANSACTION_SIZE + size;
  // DiagMsg(DIAG_DEBUG, "Code: %d msg size: %d\n", code, msg_size);
  // const int rmi = zmq_msg_init_size(&msg, msg_size);
  // if (rmi != 0)
  // {
  //   DiagMsg(DIAG_ERROR,"ZeroMQ error occurred during zmq_msg_init_size(): %s\n", zmq_strerror(errno));
  //   zmq_msg_close(&msg);
  //   return EXIT_FAILURE;
  // }
  // pmsg = zmq_msg_data(&msg);
  // *pmsg++ = code;
  // *pmsg++ = ' ';
  // memcpy(pmsg, (void*)&id, TRANSACTION_SIZE);
  // pmsg += TRANSACTION_SIZE;
  // memcpy(pmsg, buffer, size);

  // const size_t rs = zmq_msg_send(&msg, socket_com.publisher, 0);
  // if (rs != msg_size)
  // {
  //   DiagMsg(DIAG_ERROR, "ZeroMQ error occurred during zmq_msg_send(): %s\n", zmq_strerror(errno));
  //   zmq_msg_close(&msg);
  //   return EXIT_FAILURE;
  // }

  // zmq_msg_close(&msg);
  // DiagMsg(DIAG_DEBUG, "Msg Sent (Code: %d; msg size: %d)\n", code, msg_size);

  // sleep(2);
  //return EXIT_SUCCESS;
//}
// {
    //const unsigned int REPETITIONS = 10;
//     const unsigned int PACKET_SIZE = 18;
// // //   //const char *TOPIC = "a";
//     uint8_t TOPIC = 10;
// //   // void *context = zmq_ctx_new();

//    srand(time(NULL));   // Initialization, should only be called once.

//   // if (!context)
//   // {
//   //   printf("ERROR: ZeroMQ error occurred during zmq_ctx_new(): %s\n", zmq_strerror(errno));
//   //   return EXIT_FAILURE;
//   // }

//   // void *data_socket = zmq_socket(context, ZMQ_PUB);
//   void* data_socket = socket_com.publisher;
//   // const int rb = zmq_bind(data_socket, "tcp://127.0.0.1:5585");

//   // if (rb != 0)
//   // {
//   //   printf("ERROR: ZeroMQ error occurred during zmq_ctx_new(): %s\n", zmq_strerror(errno));
//   //   return EXIT_FAILURE;
//   // }

//   //const size_t topic_size = strlen(TOPIC);
//     const size_t topic_size = 1;
//   //const size_t envelope_size = topic_size + 1 + PACKET_SIZE * sizeof(int16_t);
//     const size_t envelope_size = topic_size + 1 + PACKET_SIZE;

//   //printf("Topic: %s; topic size: %zu; Envelope size: %zu\n", TOPIC, topic_size, envelope_size);

//   for (unsigned int i = 0; i < 4; i++)
//   {
//     //int16_t buffer[PACKET_SIZE];

//     // for (unsigned int j = 0; j < PACKET_SIZE; j++)
//     // {
//     //   buffer[j] = (uint8_t)rand();
//     // }

//     //printf("Read %u data values\n", PACKET_SIZE);

//     zmq_msg_t envelope;

//     const int rmi = zmq_msg_init_size(&envelope, envelope_size);
//     if (rmi != 0)
//     {
//       printf("ERROR: ZeroMQ error occurred during zmq_msg_init_size(): %s\n", zmq_strerror(errno));

//       zmq_msg_close(&envelope);

//       //break;
//     }

//     memcpy(zmq_msg_data(&envelope), &TOPIC, topic_size);
    
//     memcpy((void *)((char *)zmq_msg_data(&envelope) + topic_size), " ", 1);
//     //memcpy((void *)((char *)zmq_msg_data(&envelope) + 1 + topic_size), buffer, PACKET_SIZE * sizeof(int16_t));
//     pmsg = (void *)((char *)zmq_msg_data(&envelope) + 1 + topic_size );
//     memcpy(pmsg, &id, TRANSACTION_SIZE);
//     pmsg += TRANSACTION_SIZE;
//     memcpy(pmsg, buffer, size);
//     //memcpy((void *)((char *)zmq_msg_data(&envelope) + 1 + topic_size), buffer, PACKET_SIZE);// * sizeof(int16_t));

//     const size_t rs = zmq_msg_send(&envelope, data_socket, 0);
//     if (rs != envelope_size)
//     {
//       printf("ERROR: ZeroMQ error occurred during zmq_msg_send(): %s\n", zmq_strerror(errno));
//       zmq_msg_close(&envelope);
//       //break;
//     }

//     zmq_msg_close(&envelope);


//     //printf("Message sent; i: %u, topic: %s\n", i, TOPIC);

//     sleep(1);
//   }
  sleep(5);
  close_sockets();

  //const int rc = zmq_close(data_socket);

  // if (rc != 0)
  // {
  //   printf("ERROR: ZeroMQ error occurred during zmq_close(): %s\n", zmq_strerror(errno));

  //   return EXIT_FAILURE;
  // }

  // const int rd = zmq_ctx_destroy(context);

  // if (rd != 0)
  // {
  //   printf("Error occurred during zmq_ctx_destroy(): %s\n", zmq_strerror(errno));

  //   return EXIT_FAILURE;
  // }
#endif

   return EXIT_SUCCESS;
}

