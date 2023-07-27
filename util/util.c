#include "util.h"
#include <stdlib.h>
#include <string.h>


char *
strdup(
    const char * str)
{
    size_t size = strlen(str);
    char * dup = malloc(size+1);
    memcpy(dup, str, size);
    
    dup[size] = '\0';
    
    return dup;
}
