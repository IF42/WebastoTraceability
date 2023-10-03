/**
** @file endian.c
** @brief Functions for changing endianity for every primitive
** data types with more than 2-bytes length
*/
#include "endian.h"


uint16_t
endian_swap_int16(uint16_t n)
{
    return (n >> 8 | n << 8);
}


uint32_t
endian_swap_int32(uint32_t n)
{
    return (n >> 24
        | (n & 0x00FF0000) >> 8
        | (n & 0x0000FF00) << 8
        | n << 24);
}


uint64_t
endian_swap_int64(uint64_t n)
{
    return n >> 56
        | (n  &  0x00FF000000000000) >> 40
        | (n  &  0x0000FF0000000000) >> 24
        | (n  &  0x000000FF00000000) >> 8
        | (n  &  0x00000000FF000000) << 8
        | (n  &  0x0000000000FF0000) << 24
        | (n  &  0x000000000000FF00) << 40
        | n << 56;
}

