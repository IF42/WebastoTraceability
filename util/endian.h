/**
** @file endian.h
** @brief Header file for endian library
*/

#ifndef _ENDIAN_H_
#define _ENDIAN_H_

#include <stdint.h>


/**
** @brief Function for swaping bytes of 16-bits variable
*/
uint16_t
swap_endian_int16(uint16_t n);


/**
** @brief Function for swaping bytes of 32-bits variables
*/
uint32_t
swap_endian_int32(uint32_t n);


/**
** @brief Function for swaping bytes of 64-bits variables
*/
uint64_t
swap_endian_int64(uint64_t n);


/**
** @brief Generic macro for swaping bytes of 16-bits or 32-bits
** variable
*/
#define swap_endian(n)                                  \
    _Generic(                                           \
        (n)                                             \
        , int16_t : swap_endian_int16(n)                \
        , uint16_t: swap_endian_int16(n)                \
        , int32_t : swap_endian_int32(n)                \
        , uint32_t: swap_endian_int32(n)                \
        , float   : swap_endian_int32((int32_t) n)     \
        , uint64_t: swap_endian_int64(n)                \
        , int64_t : swap_endian_int64(n)                \
        , double  : swap_endian_int64((int64_t) n)) 
            
#endif


