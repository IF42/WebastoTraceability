#ifndef _ARRAY_H_
#define _ARRAY_H_

#include <stdbool.h>
#include <stddef.h>

typedef void (*Delete)(void *);


typedef struct 
{
    size_t size;
    size_t item_size;
    Delete delete;
}Array;


#define ARRAY(T) ((Array *) T)


#define Array(...)(Array){__VA_ARGS__}


Array * 
array_new(
    size_t item_size
    , size_t init_size
    , Delete delete);


Array *
array_init(
    size_t item_size
    , size_t size
    , const void * array_buffer
    , Delete delete);


Array *
array_clone(Array * array);


void 
array_delete(Array * array);


Array *
array_resize(
    Array * array
    , size_t new_size);




#endif
