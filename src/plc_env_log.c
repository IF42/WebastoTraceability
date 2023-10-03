#include "plc_env_log.h"

#include <time.h>
#include <stdlib.h>
#include <endian.h>



/* reading interval in seconds */
#define READING_INTERVAL 60


typedef struct 
{
    PLC_Thread super;
    uint8_t db_index;
    uint8_t timer;
}EnvLog;


#define EnvLog(...)(EnvLog){__VA_ARGS__}
#define ENV_LOG(T)((EnvLog*) T)



bool
env_log_run(
		PLC_Thread * self
		, Model * model
		, S7Object client)
{
	uint16_t environment[2];
	int cli_result;

	if(ENV_LOG(self)->timer >= READING_INTERVAL)
	{
		ENV_LOG(self)->timer = 0;

		if((cli_result = Cli_DBRead(
							client
							, ENV_LOG(self)->db_index
							, 0
							, sizeof(environment), environment)) != 0)
		{
			log_error(
				model->log
				, "%s:%d: Env log(%d): %s"
				, __FILE__
				, __LINE__
				, ENV_LOG(self)->db_index
				, cli_error(cli_result));

			return false;
		}

		if(model_write_environment_data(
			model
			, endian_swap(environment[0]) / 100.0
			, endian_swap(environment[1]) / 100.0) == false)
		{
			log_error(
				model->log
				, "%s:%d: Env log(%d): SQL write error"
				, __FILE__
				, __LINE__
				, ENV_LOG(self)->db_index);
		}
	}
	else
		ENV_LOG(self)->timer++;

    return true;
}


PLC_Thread *
plc_env_log_new(
	uint8_t db_index)
{
    EnvLog * self = malloc(sizeof(EnvLog));

    if(self != NULL)
    {
        *self = (EnvLog)
        {
        	.super      = PLC_Thread(env_log_run, (void(*)(PLC_Thread*)) free)
        	, .db_index = db_index
        };
    }

    return PLC_THREAD(self);
}


