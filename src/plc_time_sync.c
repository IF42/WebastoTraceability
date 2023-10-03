#include "plc_time_sync.h"

#include <time.h>
#include <stdlib.h>
#include <log.h>
#include <endian.h>


typedef struct 
{
    PLC_Thread super;

    uint8_t db_index;
    uint16_t counter;
}PLC_TimeSync;

#define PLC_TimeSync(...)(PLC_TimeSync){__VA_ARGS__}

#define PLC_TIME_SYNC(T)((PLC_TimeSync*) T)


/**
** time is synchronized every 1 hour (3600 seconds) if the run function is called every 500ms
*/
#define TIME_SYNC_INTERVAL 60*60


static bool
plc_thread2_run(
	PLC_Thread * self
	, Model * model
	, S7Object client)
{
	int cli_result;

    if(PLC_TIME_SYNC(self)->counter >= TIME_SYNC_INTERVAL)
    {
        PLC_TIME_SYNC(self)->counter = 0;

        time_t t     = time(NULL);
        struct tm tm = *localtime(&t);

        uint8_t sync[14] =
        {
        	0x01
			, 0x00
			, (tm.tm_year + 1900) >> 8
			, (tm.tm_year + 1900)
			, tm.tm_mon + 1
			, tm.tm_mday
			, tm.tm_wday
			, tm.tm_hour
			, tm.tm_min
			, tm.tm_sec
        };

        if((cli_result = Cli_DBWrite(
        					client
							, PLC_TIME_SYNC(self)->db_index
							, 0
							, sizeof(sync)
							, &sync) != 0))
        {
        	log_error(
				model->log
				, "%s:%d: RW DB(%d): %s"
				, __FILE__
				, __LINE__
				, PLC_TIME_SYNC(self)->db_index
				, cli_error(cli_result));

            return false;
        }
    }
    else
        PLC_TIME_SYNC(self)->counter++;
    
    return true;
}


PLC_Thread *
plc_time_sync_new(
    uint8_t db_index)
{
    PLC_TimeSync * self = malloc(sizeof(PLC_TimeSync));

    if(self != NULL)
    {
    	*self = (PLC_TimeSync)
		{
    		.super      = PLC_Thread(plc_thread2_run, (void(*)(PLC_Thread*)) free)
    		, .db_index = db_index
    		, .counter  = TIME_SYNC_INTERVAL
		};
    }

    return PLC_THREAD(self);
}


