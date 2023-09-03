#include "plc1_thread4.h"
#include "plc_thread_config.h"

#include <time.h>
#include <stdlib.h>
#include <log.h>
#include <endian.h>


typedef enum
{
	INIT
    , WAIT
    , FINISH
    , ERROR
	, STATE_N
}ThreadStep;


typedef struct
{
    bool is_error;
    ThreadStep step;
}ThreadResult;


#define throw_error return (ThreadResult){.is_error = true}
#define state(T) (ThreadResult){.step=T}


typedef struct
{
    uint8_t execute:1;
}Execute;


struct Cover
{
    uint16_t execute:1;
    uint16_t orientation:1;
    uint16_t spare:1;
    uint16_t empty:13;
    
    PLC_String cover_code;
    PLC_String bottle1_batch;
    PLC_String bottle2_batch;
}__attribute__((packed, aligned(1)));


typedef struct Cover Cover;


typedef struct
{
    uint8_t DONE:1;
    uint8_t ERROR:1;
}Result;


typedef struct PLC1_Thread4 PLC1_Thread4;
typedef ThreadResult(*ThreadCallback)(PLC1_Thread4*);


struct PLC1_Thread4
{
    PLC_Thread super;
    ThreadStep step;
    ThreadCallback thread_callback[STATE_N];
};


#define PLC1_THREAD4(T)((PLC1_Thread4*) T) 


static ThreadResult
step_init(PLC1_Thread4 * self)
{
	Result result = {0};

	if(Cli_DBWrite(self->super.client, PLC1_THREAD4_DB_INDEX, 0, sizeof(Result), &result) != 0)
		throw_error;
	else
		return state(WAIT);
}


static ThreadResult
step_wait(PLC1_Thread4 * self)
{
    Execute execute;
    Cover cover;

    if(Cli_DBRead(self->super.client, PLC1_THREAD4_DB_INDEX, 2, sizeof(Execute), &execute) != 0)
    	throw_error;

    if(execute.execute == false)
        return state(WAIT);
    
    if(Cli_DBRead(self->super.client, PLC1_THREAD4_DB_INDEX, 2, sizeof(Cover), &cover) != 0)
        throw_error;

    //set terminating zero character
    cover.cover_code.array[cover.cover_code.length]       = '\0';
    cover.bottle1_batch.array[cover.bottle1_batch.length] = '\0';
    cover.bottle2_batch.array[cover.bottle2_batch.length] = '\0';

    if(model_write_new_cover(
            self->super.model
            , cover.cover_code.array
            , cover.orientation
            , cover.bottle1_batch.array
            , cover.bottle2_batch.array
            , cover.spare) == true)
    {
        return state(FINISH);
    }
    else
    {
    	log_error(
			self->super.model->log
			, "%s%d: cover (%s) was not stored, "
			,__FILE__, __LINE__, cover.cover_code.array);

        return state(ERROR);
    }
}


static ThreadResult 
step_finish(PLC1_Thread4 * self)
{
    Result result = (Result){.DONE = true};
    Execute execute;
    
    if(Cli_DBWrite(self->super.client, PLC1_THREAD4_DB_INDEX, 0, sizeof(Result), &result) != 0
        || Cli_DBRead(self->super.client, PLC1_THREAD4_DB_INDEX, 2, sizeof(Execute), &execute) != 0)
    {
        throw_error;
    }

    if(execute.execute == false)
        return state(INIT);
    else 
        return state(FINISH);
}


static ThreadResult
step_error(PLC1_Thread4 * self)
{
    Result result = (Result){.ERROR = true};
    Execute execute;
    
    if(Cli_DBWrite(self->super.client, PLC1_THREAD4_DB_INDEX, 0, sizeof(Result), &result) != 0
        || Cli_DBRead(self->super.client, PLC1_THREAD4_DB_INDEX, 2, sizeof(Execute), &execute) != 0)
    {
        throw_error;
    }

    if(execute.execute == false)
        return state(INIT);
    else 
        return state(ERROR);
}


static bool
plc1_thread4_run(PLC_Thread * self)
{
    ThreadResult result;

    if(PLC1_THREAD4(self)->step < STATE_N)
    {
    	ThreadCallback callback = PLC1_THREAD4(self)->thread_callback[PLC1_THREAD4(self)->step];

    	if((result = callback(PLC1_THREAD4(self))).is_error == false)
			PLC1_THREAD4(self)->step = result.step;
		else
			return false;
    }
    else
    	PLC1_THREAD4(self)->step = INIT;

    return true;
}


PLC_Thread *
plc1_thread4_new(Model * model, S7Object plc)
{
    PLC1_Thread4 * self = malloc(sizeof(PLC1_Thread4));

    if(self != NULL)
    {
    	*self = (PLC1_Thread4)
		{
    		.super = PLC_Thread(model, plc, plc1_thread4_run, (void(*)(PLC_Thread*)) free)
    		, .step = INIT
			, .thread_callback = {step_init, step_wait, step_finish, step_error}
		};
    }

    return (PLC_Thread*) self;
}



