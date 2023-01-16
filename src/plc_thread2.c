#include "plc_thread2.h"
#include <time.h>
#include <stdlib.h>
#include <log.h>
#include <endian.h>


struct DateTime_Sync
{
    uint16_t valid:1;
    uint16_t empty:15;
    DTL date_time;
}__attribute__((packed, aligned(1)));

typedef struct DateTime_Sync DateTime_Sync;

typedef struct 
{
    PLC_Thread super;

    uint8_t db_index;
    uint8_t counter;
}PLC_Thread2;

#define PLC_Thread2(...)(PLC_Thread2){__VA_ARGS__}

#define PLC_THREAD2(T)((PLC_Thread2*) T)


static bool
plc_thread2_run(PLC_Thread * self)
{
    if(PLC_THREAD2(self)->counter >= 10)
    {
        PLC_THREAD2(self)->counter = 0;
        
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);

        DateTime_Sync sync;
        sync.valid = true;
        sync.date_time.year = tm.tm_year + 1900;
        sync.date_time.month = tm.tm_mon + 1;
        sync.date_time.day = tm.tm_mday;
        sync.date_time.weekday = tm.tm_wday;
        sync.date_time.hour = tm.tm_hour;
        sync.date_time.minute = tm.tm_min;
        sync.date_time.second = tm.tm_sec;
        sync.date_time.nanosecond = 0;

        sync.date_time.year = swap_endian(sync.date_time.year);

        if(Cli_DBWrite(self->client, PLC_THREAD2(self)->db_index, 0, sizeof(DateTime_Sync), &sync) != 0)
            return false;

        log_debug(self->model->log, "plc1 thread2 time synchronized");
    }
    else
        PLC_THREAD2(self)->counter++;    
    
    return true;
}


PLC_Thread *
plc_thread2_new(
    Model * model
    , S7Object client
    , uint8_t db_index)
{
    PLC_Thread2 * self = malloc(sizeof(PLC_Thread2));

    if(self != NULL)
    {
        self->super = PLC_Thread(model, client, plc_thread2_run, (void(*)(PLC_Thread*))free);
        self->db_index = db_index;
    }

    return PLC_THREAD(self);
}


