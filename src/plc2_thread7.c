#include "plc2_thread7.h"
#include "plc_thread_config.h"

#include <util.h>
#include <time.h>
#include <stdlib.h>
#include <endian.h>
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

typedef struct PLC2_Thread7 PLC2_Thread7;
typedef ThreadResult (*ThreadCallback)(PLC2_Thread7 *);


struct PLC2_Thread7
{
    PLC_Thread super;
    ThreadState step;
    ThreadCallback thread_callback[STATE_N];
};


#define PLC2_THREAD7(T)((PLC2_Thread7*) T)


typedef struct
{
    uint8_t execute:1;
}Execute;


typedef struct
{
    uint8_t DONE:1;
    uint8_t ERROR:1;
}Result;

typedef struct Frame Frame;

struct Frame
{
    PLC_String table;
    PLC_String frame_code;
    PLC_String PATH;
    int16_t temperature;
    int16_t humidity;
}__attribute__((packed, aligned(1)));


static ThreadResult
step_init(PLC2_Thread7 * self)
{
	 Result result = {0};

	 if(Cli_DBWrite(self->super.client, PLC2_THREAD7_DB_INDEX, 0, sizeof(Result), &result) != 0)
		 throw_error;
	 else
		 return state(WAIT);
}


static ThreadResult 
step_wait(PLC2_Thread7 * self)
{
    Execute execute;
    Frame frame;

    if(Cli_DBRead(self->super.client, PLC2_THREAD7_DB_INDEX, 2, sizeof(Execute), &execute) != 0)
       throw_error;

    if(execute.execute == false) 
        return state(WAIT);

    if(Cli_DBRead(self->super.client, PLC2_THREAD7_DB_INDEX, 4, sizeof(Frame), &frame) != 0)
        throw_error;

    frame.table.array[frame.table.length]           = '\0';
    frame.frame_code.array[frame.frame_code.length] = '\0';
    frame.PATH.array[frame.PATH.length]             = '\0';
    frame.temperature                               = swap_endian(frame.temperature);
    frame.humidity                                  = swap_endian(frame.humidity);

    if(model_generate_frame_csv(
        self->super.model
        , frame.PATH.array
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
				, "%s%d: plc2 csv was not create for frame: %s"
				,__FILE__, __LINE__, frame.frame_code.array);
        return state(ERROR);
    }
}


static ThreadResult 
step_finish(PLC2_Thread7 * self)
{
    Execute execute;
    Result result = {.DONE = true};
    
    if(Cli_DBRead(self->super.client, PLC2_THREAD7_DB_INDEX, 2, sizeof(Execute), &execute) != 0
        || Cli_DBWrite(self->super.client, PLC2_THREAD7_DB_INDEX, 0, sizeof(Result), &result) != 0)
    {
        throw_error;
    }

    if(execute.execute == false)
        return state(INIT);
    else
        return state(FINISH);
}


static ThreadResult 
step_error(PLC2_Thread7 * self)
{
    Execute execute;
    Result result = {.ERROR = true};

    if(Cli_DBRead(self->super.client, PLC2_THREAD7_DB_INDEX, 2, sizeof(Execute), &execute) != 0
        || Cli_DBWrite(self->super.client, PLC2_THREAD7_DB_INDEX, 0, sizeof(Result), &result) != 0)
    {
        throw_error;
    }

    if(execute.execute == false)
        return state(INIT);
    else
        return state(ERROR);
}


bool
plc2_thread7_run(PLC_Thread * self)
{
    ThreadResult result;

    if(PLC2_THREAD7(self)->step < STATE_N)
    {
    	ThreadCallback thread_callback = PLC2_THREAD7(self)->thread_callback[PLC2_THREAD7(self)->step];

    	if((result = thread_callback(PLC2_THREAD7(self))).is_error == false)
			PLC2_THREAD7(self)->step = result.step;
		else
			return false;
    }
    else
    	PLC2_THREAD7(self)->step = INIT;

    return true;
}


PLC_Thread *
plc2_thread7_new(
    Model * model
    , S7Object client)
{
    PLC2_Thread7 * self = malloc(sizeof(PLC2_Thread7));

    if(self != NULL)
    {
    	*self = (PLC2_Thread7)
		{
    		.super = PLC_Thread(model, client, plc2_thread7_run, (void(*)(PLC_Thread*)) free)
        	, .step  = INIT
			, .thread_callback = {step_init, step_wait, step_finish, step_error}
		};
    }

    return PLC_THREAD(self);
}


