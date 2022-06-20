

#include "socket_term.h"
#include "socket_com.h"
#include "ftdi_connect.h"
#include "diagnostics_util.h"
#include "util_common.h"
#include "unistd.h"
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "zmq.h"



#define MAX_COMMAD_SIZE 20

typedef struct Socket_Term
{
    const char *connectstr;
    pthread_t threadId;
    char sport[20];
    int32_t iport;
    char sn[20];
    uint32_t baud;
    volatile bool stop;
}Socket_Term_t;

Socket_Term_t socket_term;

void *socket_menu(void *arg);


void init_socket_menu(const char * connectstr)
{
  uint32_t ret;
  pthread_attr_t attr;
  struct sched_param sp;
  

  socket_term.connectstr = connectstr;
  ret = pthread_attr_init(&attr);

  if (pthread_attr_getschedparam(&attr, &sp))
    DiagMsg(DIAG_ERROR, "send receive queue thread attrib prio failed ");
  else
    DiagMsg(DIAG_DEBUG, "Socket Menu thread priority %d", sp.sched_priority);

  if (ret)
    DiagMsg(DIAG_ERROR, "send receive queue thread attrib init failed (%d)", ret);
  
  socket_term.stop = false;
  ret = pthread_create(&(socket_term.threadId), &attr, &socket_menu, (void *)&socket_term);
}

void close_socket_menu(void)
{
  socket_term.stop = true;
}

void *socket_menu(void *arg)
{
  char buffer [100];
  char recconnectstr[89];
  char sendconnectstr[89];
  uint32_t size = 0, len;
  int32_t dev, port;
  void *context = zmq_ctx_new ();
  void *responder = zmq_socket (context, ZMQ_REP);
  char * token;
  Socket_Term_t * pSTerm = (Socket_Term_t *)arg;
  char *p;
  bool sn_config = false, port_config = false, dev_config = false, connected;

  memset(pSTerm->sport, 0 , sizeof(pSTerm->sport));
  memset(pSTerm->sn, 0 , sizeof(pSTerm->sn));
  pSTerm->baud = 115200;

  memset(buffer, 0 , sizeof(buffer));

  int rc = zmq_bind (responder, pSTerm->connectstr);

  if(rc)
  {
    DiagMsg(DIAG_ERROR, "Failed start socket menu %s, %d", pSTerm->connectstr, rc);
    pSTerm->stop = true;
  }
  else
  {
    DiagMsg(DIAG_INFO, "Socket Menu thread started. 0x%X %s" , pSTerm->threadId, pSTerm->connectstr);
  }

  token = strrchr(pSTerm->connectstr, ':');

  if(token)
  {
    //memcpy((void*)recconnectstr, pSTerm->connectstr, token - pSTerm->connectstr);
    *token = '\0';
    token++;
    port = atoi(token);
    port += 10;
    sprintf(recconnectstr,"%s:%d", pSTerm->connectstr, port);
    port += 10;
    sprintf(sendconnectstr,"%s:%d", pSTerm->connectstr, port);
    //DiagMsg(DIAG_DEBUG, "receiver connect str %s" , recconnectstr);
    //init_socket_receiver(recconnectstr);
  }
  else
    DiagMsg(DIAG_ERROR, "Failed to create receive Connection string (%s)", pSTerm->connectstr);

  while(!pSTerm->stop)
  {
    memset(buffer, 0 , sizeof(buffer));
    zmq_recv (responder, buffer, MAX_COMMAD_SIZE, 0);
    if(!strncasecmp(buffer, "Check", 5))
    {
      DiagMsg(DIAG_INFO, "Check");
      size = sprintf(buffer,"ACK");      
    }
    else if(!strncasecmp(buffer, "dev", 3))
    {
      dev_config = false;
      DiagMsg(DIAG_INFO, "Configure %s", buffer );
      p = &buffer[4];
      dev = getInt(p, &dev_config);
    }
    else if(!strncasecmp(buffer, "port", 4))
    {
      port_config = false;
      DiagMsg(DIAG_INFO, "Configure %s", buffer );
      p = &buffer[5];
      len = strlen(p);
      if(len < sizeof(pSTerm->sport))
      {
        port_config = true;
        memcpy((void*)pSTerm->sport, p, len);
        port = getInt(pSTerm->sport, &port_config);
        if(port_config)
        {
          pSTerm->iport = port;
          size = sprintf(buffer,"ACK COM%d", pSTerm->iport);
        }
        else
        {
          size = sprintf(buffer,"NACK Illegal int port"); 
        }
      }
      else
        size = sprintf(buffer,"NACK Illegal portname");      
    }
    else if(!strncasecmp(buffer, "sn", 2))
    {
      sn_config = false; 
      DiagMsg(DIAG_INFO, "Configure %s", buffer );
      p = &buffer[3];
      len = strlen(p);
      if(len < sizeof(pSTerm->sn))
      {
        sn_config = true;
        memcpy((void*)pSTerm->sn, p, len);
        size = sprintf(buffer,"ACK %s", pSTerm->sn);
      }
      else
        size = sprintf(buffer,"NACK Illegal sn");  
    }
    else if(!strncasecmp(buffer, "baud", 4))
    {
      p = &buffer[5];
      len = atoi(p); 
      DiagMsg(DIAG_INFO, "Configure baud %d", len );
      if(len)
      {
        pSTerm->baud = len;
        size = sprintf(buffer,"ACK baud %d", pSTerm->baud);
      }
      else
        size = sprintf(buffer,"NACK Illegal baud");  
    }
    else if(!strncasecmp(buffer, "reccon", 6))
    {
      p = &buffer[7];
      len = strlen(p);
      if(len < sizeof(recconnectstr))
      {
        memcpy((void*)recconnectstr, p, len);
        size = sprintf(buffer,"ACK %s", recconnectstr);
      }
      else
        size = sprintf(buffer,"NACK Illegal rec con str");  

      DiagMsg(DIAG_INFO, "Configure receive port %s", recconnectstr );
    }
    else if(!strncasecmp(buffer, "sencon", 6))
    {
      p = &buffer[7];
      len = strlen(p);
      if(len < sizeof(sendconnectstr))
      {
        memcpy((void*)sendconnectstr, p, len);
        size = sprintf(buffer,"ACK %s", sendconnectstr);
      }
      else
        size = sprintf(buffer,"NACK Illegal send con str");  

      DiagMsg(DIAG_INFO, "Configure send port %s", sendconnectstr );
    }
    else if(!strncasecmp(buffer, "connect", 7))
    {
      DiagMsg(DIAG_INFO, "Open device...");
      connected = false;
      if(dev_config)
      {
        if((connect(dev, pSTerm->baud)))
        {
          DiagMsg(DIAG_ERROR, "Failed to connect to device num %d", dev);
        }
        else
        {          
          connected = true;
          size = sprintf(buffer,"ACK");
        }
      }
      if(sn_config && !connected)
      {
        if(get_device_sn(pSTerm->sn, &dev))
        {
          if((connect(dev, pSTerm->baud)))
          {
            DiagMsg(DIAG_ERROR, "Failed to connect to device num %d (sn %s)", dev, pSTerm->sn);
          }
          else
          {
            connected = true;
            size = sprintf(buffer,"ACK");
          }
        }
        else
        {
          DiagMsg(DIAG_WARNING, "SN not found %s" , pSTerm->sn);
        }
      }
      if(port_config && !connected)
      {
        
        if(get_device_port(pSTerm->iport, &dev))
        {
            if((connect(dev, pSTerm->baud)))
            {
              DiagMsg(DIAG_ERROR, "Failed to connect to device num %d (sn %s)", dev, pSTerm->sn);
            }
            else
            {
              connected = true;
              size = sprintf(buffer,"ACK");
            }
        }
        else
        {
          DiagMsg(DIAG_WARNING, "Port not found %d" , pSTerm->iport);
        }
      }
      if(!connected)
        size = sprintf(buffer,"NACK");
      else
      {
        init_socket_receiver(recconnectstr);
        init_socket_sender(sendconnectstr);
        DiagMsg(DIAG_INFO,"Connected to %s (dev %d, baud %d, port COM%d)", pCurrentDev->pDevInfo->SerialNumber, pCurrentDev->devid, pCurrentDev->baud, pCurrentDev->port);
      }
            
    }
    else if(!strncasecmp(buffer, "close", 5))
    {
      DiagMsg(DIAG_INFO, "Close");
      
      close_device();
      size = sprintf(buffer,"ACK");
      close_sockets();

    }
    else
    {
      DiagMsg(DIAG_WARNING, "Unknown Msg %s", buffer);
      size = sprintf(buffer,"NACK");
    }
    zmq_send (responder, buffer, size, 0);
  }
  zmq_close (responder);
  zmq_ctx_destroy (context);
  DiagMsg(DIAG_INFO, "Socket Menu closed. ");
  return NULL;
}

