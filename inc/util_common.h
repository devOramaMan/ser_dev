
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include "config.h"

#ifndef UTIL_COMMON
#define UTIL_COMMON


#ifdef NO_CLEAR
#define CLEAR_SCREEN()
#else
#ifdef _WIN32
#ifdef __CYGWIN__
//#define  CLEAR_SCREEN() system("clear")
// lets not loose logging information for now don't clear
#define  CLEAR_SCREEN()
#define  CLEAR_SCREEN() system("clear")
#else
#define  CLEAR_SCREEN() system("cls")
#endif
#elif __linux__
#define  CLEAR_SCREEN() system("clear")
#else
#define CLEAR_SCREEN()
#endif

#endif



int32_t isNumber(char * s, int32_t size);
float isFloat(char *str, bool *ok);
int32_t getInt(char *str, bool *ok);
uint32_t setDataType(uint8_t datatype, uint8_t * buffer, uint8_t * val);
int checkAscii (uint8_t *array, uint32_t size);

#endif 

