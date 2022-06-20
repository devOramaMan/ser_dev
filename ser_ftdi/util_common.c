

#include "util_common.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <protocol_common.h>

inline int32_t isNumber(char * s, int32_t size)
{
	int32_t i;
    for (i= 0; s[i]!= '\0' && i < size; i++)
    {
        if (isdigit((int)s[i]) == 0)
              return 0;
    }
    return 1;
}

inline float isFloat(char *str, bool *ok)
{
  int32_t len;
  float val;
  int32_t ret;
  ret = sscanf(str, "%f %n", &val, &len);
  *ok = (ret && len == strlen(str)) ? true : false;
  return *ok ? val : 0.0;
}

inline int32_t getInt(char *lstr, bool *ok)
{
  int32_t len;
  int32_t val;
  int32_t ret;
  char *str = lstr;

  // loop to the first number
  while (!(*str >= '0' && *str <= '9') && (*str != '-') && (*str != '+')) str++;
    
  ret = sscanf(str, "%d %n", &val, &len);
  *ok = (ret && len == strlen(str)) ? true : false;

  return *ok ? val : 0;
}

inline uint32_t setData(uint8_t datatype, uint8_t * buffer, uint8_t * val)
{
  uint32_t size, i;
  uint8_t *lval = val;
  switch ((Data_Type)datatype)
  {
    case INTEGER8:
    case UNSIGNED8:
    size = 1;
    break;
  case INTEGER16:
  case UNSIGNED16:
    size = 2;
    break;
  case INTEGER32:
  case UNSIGNED32:
  case REAL32:
    size = 4;
    break;
  default:
    return 0;
  }
  for(i=0; i<size;i++)
    *buffer++ = *lval++;
  return size;
}
