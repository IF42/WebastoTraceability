#include "plc1_thread11.h"
#include "plc_thread_config.h"

#include <util.h>
#include <stdlib.h>
#include <string.h>


typedef enum
{
	INIT
    , WAIT
    , FINISH
    , ERROR
	, STATE_N
}ThreadState;


typedef struct
{
    bool is_error;
    ThreadState step;
}ThreadResult;

#define throw_error return (ThreadResult){.is_error = true}
#define state(T)(ThreadResult){.step=T}

typedef struct PLC1_Thread11 PLC1_Thread11;
typedef ThreadResult (*ThreadCallback)(PLC1_Thread11 *);


struct PLC1_Thread11
{
    PLC_Thread super;
    ThreadState step;
    ThreadCallback thread_callback[STATE_N];
};

#define PLC1_THREAD11(T)((PLC1_Thread11*)T)


typedef struct
{
    uint8_t execute:1;
}Execute;


typedef struct
{
    uint8_t DONE:1;
    uint8_t ERROR:1;
}Result;


typedef struct
{
    PLC_String table;
    PLC_String frame_code;
    PLC_String path;
    int16_t temperature;
    int16_t humidity;
}Frame;


static ThreadResult
step_init(PLC1_Thread11 * self)
{
	Result result = {0};

	if(Cli_DBWrite(self->super.client, PLC1_THREAD11_DB_INDEX, 0, sizeof(Result), &result) != 0)
		throw_error;
	else
		return state(WAIT);

}


static ThreadResult 
step_wait(PLC1_Thread11 * self)
{
    Execute execute;
    Frame frame;

    if(Cli_DBRead(self->super.client, PLC1_THREAD11_DB_INDEX, 2, sizeof(Execute), &execute) != 0)
        throw_error;

    if(execute.execute == false) 
        return state(WAIT);

    if(Cli_DBRead(self->super.client, PLC1_THREAD11_DB_INDEX, 4, sizeof(Frame), &frame) != 0)
        throw_error;

    frame.table.array[frame.table.length]           = '\0';
    frame.frame_code.array[frame.frame_code.length] = '\0';
    frame.path.array[frame.path.length]             = '\0';

    if(model_generate_frame_csv(
        self->super.model
        , frame.path.array
        , frame.table.array
        , frame.frame_code.array
        , ((float)frame.temperature)/10.0
        , ((float)frame.humidity)/10.0) == true)
    {
        return state(FINISH);
    }
    else
    {
    	log_error(
			self->super.model->log
			, "%s%d: plc1 csv was not create for frame: %s"
			,__FILE__, __LINE__, frame.frame_code.array);
        return state(ERROR);
    }
}


static ThreadResult
step_finish(PLC1_Thread11 * self)
{
    Execute execute;
    Result result = {.DONE = true};
    
    if(Cli_DBRead(self->super.client, PLC1_THREAD11_DB_INDEX, 2, sizeof(Execute), &execute) != 0
        || Cli_DBWrite(self->super.client, PLC1_THREAD11_DB_INDEX, 0, sizeof(Result), &result) != 0)
    {
       throw_error;
    }

    if(execute.execute == false)
        return state(INIT);
    else
        return state(FINISH);
}


static ThreadResult 
step_error(PLC1_Thread11 * self)
{
    Execute execute;
    Result result = {.ERROR = true};

    if(Cli_DBRead(self->super.client, PLC1_THREAD11_DB_INDEX, 2, sizeof(Execute), &execute) != 0
        || Cli_DBWrite(self->super.client, PLC1_THREAD11_DB_INDEX, 0, sizeof(Result), &result) != 0)
    {
        throw_error;
    }

    if(execute.execute == false)
        return state(WAIT);
    else
        return state(ERROR);
}


static bool
thread_run(PLC_Thread * self)
{
    ThreadResult result;

    if(PLC1_THREAD11(self)->step < STATE_N)
    {
    	ThreadCallback callback = PLC1_THREAD11(self)->thread_callback[PLC1_THREAD11(self)->step];

    	if((result = callback(PLC1_THREAD11(self))).is_error == false)
			PLC1_THREAD11(self)->step = result.step;
		else
			return false;
    }
    else
    	PLC1_THREAD11(self)->step = INIT;

    return true;
}


PLC_Thread *
plc1_thread11_new(
    Model * model
    , S7Object client)
{
    PLC1_Thread11 * self = malloc(sizeof(PLC1_Thread11));

    if(self != NULL)
    {
    	*self = (PLC1_Thread11)
		{
			.super = PLC_Thread(model, client, thread_run, (void(*)(PLC_Thread*)) free)
			, .step = INIT
			, .thread_callback = {step_init, step_wait, step_finish, step_error}
		};
    }

    return PLC_THREAD(self);
}



