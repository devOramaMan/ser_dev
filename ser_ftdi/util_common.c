

#include "util_common.h"

int32_t isNumber(char * s, int32_t size)
{
	int32_t i;
    for (i= 0; s[i]!= '\0' && i < size; i++)
    {
        if (isdigit((int)s[i]) == 0)
              return 0;
    }
    return 1;
}