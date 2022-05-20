


#include "diagnostics_util.h"
#include <stdio.h>                      /* standard I/O .h-file               */
#include <string.h>
#include <ctype.h>                      /* character functions                */
#include <time.h>
#include <stdarg.h>


/* Defines ----------------------------------------------------------------*/

#define DIAG_MS 0
#define DIAG_US 1

#define MAX_PRINT_SIZE 122


/* Local func -------------------------------------------------------------*/
uint16_t addLocalTime(char* print);
uint16_t addDiagnosticType(char* print, DiagnosticType type);
uint16_t parseClassName(char* print, const char * className);
void printDiag(DiagnosticType type, const char * str, ...);


/* Local var --------------------------------------------------------------*/
volatile FS_CALLBACK pFSCallback = printDiag;
volatile uint32_t lastClock = 0;
volatile uint32_t verbose = (uint32_t) DIAG_ERROR;

void diag_set_verbose(uint32_t val)
{
  verbose = val;
}

uint16_t addLocalTime(char* print)
{
  uint16_t ret;
  struct timespec tp;
  clock_gettime(CLOCK_REALTIME, &tp);

  struct tm *info;
  time_t curtime = tp.tv_sec;
  info = localtime(&curtime);
  #if (DIAG_MS == 1) && (DIAG_US == 0)
  ret = snprintf(print, 17,"%02d:%02d:%02d:%03d - ", info->tm_hour, info->tm_min, info->tm_sec, (uint32_t)tp.tv_nsec/1000000);
  #endif
  #if (DIAG_US == 1)
  ret = snprintf(print, 20,"%02d:%02d:%02d:%06d - ", info->tm_hour, info->tm_min, info->tm_sec, (uint32_t)tp.tv_nsec/1000);
  #endif

  return ret;
}

uint16_t addDiagnosticType(char* print, DiagnosticType type)
{
	static const char DiagnosticTypeStr[][8] = {"ERROR: ", "WARN:  ",  "INFO:  ", "DEBUG: ", "UNKN:  ", "RXMSG: ", "TXMSG: "};
  memcpy(print, DiagnosticTypeStr[type], 7);
  return 7;
}

uint16_t parseClassName(char* print, const char * className)
{
  uint16_t i = 0; //Skip PN7
  uint8_t j = 0;
  char tmp[50] = {0};
  uint8_t size = strlen(className);
  
  memset(tmp, 0, sizeof(tmp));
  
  if(size >= sizeof(tmp))
    size = sizeof(tmp);
  
  while( i < size)
  {
    if((className[i] >= '0') && (className[i] <= '9'))
    {
      memset(tmp, 0, j);
      j = 0; 
    }
    else
    {
      tmp[j++] = className[i];
    }
    i++;    
  }
  
  i = 0;
  while( (tmp[i] != 0x00) && (i < size) )
  {
    print[i] = tmp[i];
    i++;
  }
  
  
  if( i == 1 )
  {
    print[0] = 'N';
    print[1] = 'U';
    print[2] = 'L';
    print[3] = 'L';
    i = 4;
  }

  print[i-1] = ' ';
  print[i++] = '-';
  print[i++] = ' ';
  
  if(i > 30)
    return 30;
  return i;
}

void printDiag(DiagnosticType type, const char * str, ...)
{	
  DiagnosticType ltype = type;
  uint32_t i;
  uint16_t len = 0;
  uint16_t snlen;

  if(type > verbose)
    return;

  if(type > DIAG_UNKNOWN)
    ltype = DIAG_UNKNOWN;

  char print[MAX_PRINT_SIZE];
  for(i = 0; i < MAX_PRINT_SIZE; i++)
    print[i] = 0;
  
    
  len += addLocalTime(&print[len]);
  len += addDiagnosticType(&print[len], ltype);
  //len += parseClassName(&print[len], className);
  
  //Remaining space for snprinft args
  snlen =  (sizeof(print) - len);  
  if(snlen >= sizeof(print))
  {
    printf("Diag - Size Error (len: %d)\n", len);
    return;
  }
  
  va_list pArgs;
  va_start(pArgs, str);
    
  len += (uint16_t)vsnprintf(&print[len], snlen,  str , pArgs);
   
  printf("%s\n", print);
  
  
  va_end(pArgs);
}