#include "array.h"
#include <stdlib.h>
#include <string.h>


Array *
array_new(
    size_t item_size
    , size_t init_size
    , Delete delete)
{
    if(item_size == 0)
        return NULL;

    Array * array = malloc((sizeof(Array)) + (item_size * init_size));

    if(array == NULL) return NULL;

    *array = Array(init_size, item_size, delete);

    return array;
}


Array *
array_init(
    size_t item_size
    , size_t size
    , const void * array_buffer
    , Delete delete)
{
    if(item_size == 0)
        return NULL;

    Array * array = 
        malloc(sizeof(Array) + (item_size * size));

    if(array == NULL) return NULL;

    *array = Array(size, item_size, delete);
    
    memcpy(
        ((char*) array + sizeof(Array))
        , array_buffer
        , size * item_size);
    
    return array;
}


void
array_delete(Array * array)
{
    array->delete(array);
}


Array *
array_clone(Array * array)
{
    size_t byte_size = sizeof(Array) + (array->size * array->item_size);
    
    Array * clone = malloc(byte_size);

    if(clone == NULL) return NULL;

    memcpy(clone, array, byte_size);

    return clone;
}


Array *
array_resize(
    Array * array
    , size_t new_size)
{
    Array * resized_array = 
        malloc(sizeof(Array) + (array->item_size * new_size));

    if(resized_array == NULL) return array;

    *resized_array = Array(new_size, array->item_size, array->delete);

    memcpy(
        ((char*) resized_array + sizeof(Array))
        , ((char*) array + sizeof(Array))
        , array->item_size * array->size);
 
    free(array);

    return resized_array;
}



