#include "plc_ping.h"

#include <stdlib.h>


typedef struct 
{
    PLC_Thread super;

    uint8_t db_index;
    uint8_t counter;
}PLC_Ping;


#define PLC_Ping(...)(PLC_Ping){__VA_ARGS__}
#define PLC_PING(T)((PLC_Ping*) T)


static bool
plc_ping_run(
	PLC_Thread * self
	, Model * model
	, S7Object client)
{
	int cli_result;
    uint8_t ping;

    if(PLC_PING(self)->counter >= 1)
    {
        PLC_PING(self)->counter = 0;

        if((cli_result = Cli_DBRead(
        						client
								, PLC_PING(self)->db_index
								, 0
								, sizeof(ping)
								, &ping)) != 0)
        {
        	log_error(
				model->log
				, "%s:%d: RW DB(%d): %s"
				, __FILE__
				, __LINE__
				, PLC_PING(self)->db_index
				, cli_error(cli_result));

            return false;
        }

        if((ping & 0x01) == false)
        {
            ping = 0x01;
            
            if((cli_result = Cli_DBWrite(
            					client
								, PLC_PING(self)->db_index
								, 0
								, sizeof(ping)
								, &ping)) != 0)
            {
            	log_error(
					model->log
					, "%s:%d: RW DB(%d): %s"
					, __FILE__
					, __LINE__
					, PLC_PING(self)->db_index
					, cli_error(cli_result));

                return false;
            }
        }
    }
    else
        PLC_PING(self)->counter++;    

    return true;
}


PLC_Thread *
plc_ping_new(
    uint8_t db_index)
{
    PLC_Ping * self = malloc(sizeof(PLC_Ping));

    if(self != NULL)
    {
    	*self = (PLC_Ping)
		{
    		.super = PLC_Thread(plc_ping_run, (void(*)(PLC_Thread*)) free)
    		, .db_index = db_index
		};
    }

    return PLC_THREAD(self);
}



