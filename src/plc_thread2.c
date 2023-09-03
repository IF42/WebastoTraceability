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
    uint16_t counter;
}PLC_Thread2;

#define PLC_Thread2(...)(PLC_Thread2){__VA_ARGS__}

#define PLC_THREAD2(T)((PLC_Thread2*) T)


/**
** time is synchronized every 1 hour (3600 seconds) if the run function is called every 500ms
*/
#define TIME_SYNC_INTERVAL 60*60


static bool
plc_thread2_run(PLC_Thread * self)
{
    if(PLC_THREAD2(self)->counter >= TIME_SYNC_INTERVAL)
    {
        PLC_THREAD2(self)->counter = 0;
        
        time_t t     = time(NULL);
        struct tm tm = *localtime(&t);

        DateTime_Sync sync =
        {
         .valid = true
        , .date_time =
        	{
			.year         = swap_endian((tm.tm_year + 1900))
			, .month      = tm.tm_mon + 1
			, .day        = tm.tm_mday
			, .weekday    = tm.tm_wday
			, .hour       = tm.tm_hour
			, .minute     = tm.tm_min
			, .second     = tm.tm_sec
        	}
        };

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
        self->super    = PLC_Thread(model, client, plc_thread2_run, (void(*)(PLC_Thread*)) free);
        self->db_index = db_index;
        self->counter  = TIME_SYNC_INTERVAL;
    }

    return PLC_THREAD(self);
}


