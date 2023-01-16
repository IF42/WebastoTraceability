#ifndef _PLC_THREAD_H_
#define _PLC_THREAD_H_ 

#include "model.h"

#include <snap7.h>
#include <stdbool.h>


typedef struct
{
    uint8_t size;
    uint8_t length;
    char array[254];
}PLC_String;


typedef struct
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t weekday;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint32_t nanosecond;
}DTL;

typedef struct PLC_Thread PLC_Thread;


struct PLC_Thread
{
    Model * model;
    S7Object client;

    bool (*callback)(PLC_Thread *);
    void (*delete)(PLC_Thread *);
};


#define PLC_Thread(...)(PLC_Thread){__VA_ARGS__}

#define PLC_THREAD(T)((PLC_Thread*) T)


bool
plc_thread_run(PLC_Thread * self);


void
plc_thread_delete(PLC_Thread * self);


#endif
