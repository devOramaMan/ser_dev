
#include <stdint.h>
#include <ctype.h>
#include "config.h"

#ifndef UTIL_COMMON
#define UTIL_COMMON


#ifdef NO_CLEAR
#define CLEAR_SCREEN()
#else
#ifdef _WIN32
#ifdef __CYGWIN__
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


#endif 

