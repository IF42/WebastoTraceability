#include "plc1_thread3.h"

#include <time.h>
#include <stdlib.h>
#include <log.h>
#include <endian.h>


#define DB_INDEX 6


typedef enum
{
    WAIT
    , FINISH_TRUE
    , FINISH_FALSE
    , ERROR
}ThreadStep;


typedef struct
{
    bool is_error;
    ThreadStep step;
}ThreadResult;

#define ThreadResult(...)(ThreadResult){__VA_ARGS__}


typedef struct
{
    uint8_t execute:1;
}Execute;


typedef struct
{
    uint8_t DONE:1;
    uint8_t ERROR:1;
    uint8_t COVER_EXISTS:1;
}Result;


typedef struct 
{
    PLC_Thread super;
    ThreadStep step;
}PLC1_Thread3;


#define PLC1_THREAD3(T)((PLC1_Thread3*) T)


static ThreadResult
step_wait(PLC1_Thread3 * self)
{
    Result result = {0};
    Execute execute;
    PLC_String cover_code;

    if(Cli_DBWrite(self->super.client, DB_INDEX, 0, sizeof(Result), &result) != 0 
        || Cli_DBRead(self->super.client, DB_INDEX, 2, sizeof(Execute), &execute) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);
        
    if(Cli_DBRead(self->super.client, DB_INDEX, 4, sizeof(PLC_String), &cover_code) != 0)   
        return ThreadResult(.is_error = true);

    cover_code.array[cover_code.length] = '\0';

    if(model_check_cover_exists(self->super.model, cover_code.array) == true)
        return ThreadResult(.step = FINISH_TRUE);
    else
        return ThreadResult(.step = FINISH_FALSE);
}


static ThreadResult 
step_finish(PLC1_Thread3 * self)
{
    Execute execute;
    Result result;

    result.DONE = true;
    result.ERROR = false;
    
    if(PLC1_THREAD3(self)->step == FINISH_TRUE)
        result.COVER_EXISTS = true;
    
    if(Cli_DBWrite(self->super.client, DB_INDEX, 0, sizeof(Result), &result) != 0
        || Cli_DBRead(self->super.client, DB_INDEX, 2, sizeof(Execute), &execute) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);
    else
        return ThreadResult(.step = self->step);
}

static ThreadResult
step_error(PLC1_Thread3 * self)
{
    Result  result = {.ERROR = true};            
    Execute execute;

    if(Cli_DBWrite(self->super.client, DB_INDEX, 0, sizeof(Result), &result) != 0
        || Cli_DBRead(self->super.client, DB_INDEX, 2, sizeof(Execute), &execute) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);
    else
        return ThreadResult(.step = ERROR);
}



static bool
plc1_thread3_run(PLC_Thread * self)
{
    ThreadResult result;

    switch(PLC1_THREAD3(self)->step)
    {
        case WAIT:
            if((result = step_wait(PLC1_THREAD3(self))).is_error == false)
                PLC1_THREAD3(self)->step = result.step;
            else
                return false;

            break;
        case FINISH_TRUE:
        case FINISH_FALSE:
            if((result = step_finish(PLC1_THREAD3(self))).is_error == false)
                PLC1_THREAD3(self)->step = result.step;
            else
                return false;

            break;
        case ERROR:
            if((result = step_error(PLC1_THREAD3(self))).is_error == false)
                PLC1_THREAD3(self)->step = result.step;
            else
                return false;
            
            break;
        default:
            PLC1_THREAD3(self)->step = WAIT;
    }

    return true;
}


PLC_Thread *
plc1_thread3_new(Model * model, S7Object client)
{
    PLC1_Thread3 * self = malloc(sizeof(PLC1_Thread3));

    if(self != NULL)
    {
        self->super = PLC_Thread(model, client, plc1_thread3_run, (void(*)(PLC_Thread *)) free);
        self->step = WAIT;
    }

    return PLC_THREAD(self);
}




