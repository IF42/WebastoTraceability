#include "plc1_thread4.h"
#include "plc_thread_config.h"

#include <time.h>
#include <stdlib.h>
#include <log.h>
#include <endian.h>



typedef enum
{
    WAIT
    , FINISH
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


typedef struct 
{
    PLC_Thread super;
    ThreadStep step;
}PLC1_Thread4;


#define PLC1_THREAD4(T)((PLC1_Thread4*) T) 

static ThreadResult
step_wait(PLC1_Thread4 * self)
{
    Result result = {0};
    Execute execute;
    Cover cover;

    if(Cli_DBWrite(self->super.client, PLC1_THREAD4_DB_INDEX, 0, sizeof(Result), &result) != 0
        || Cli_DBRead(self->super.client, PLC1_THREAD4_DB_INDEX, 2, sizeof(Execute), &execute) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);
    
    if(Cli_DBRead(self->super.client, PLC1_THREAD4_DB_INDEX, 2, sizeof(Cover), &cover) != 0)
        return ThreadResult(.is_error = true);

    //set terminating zero character
    cover.cover_code.array[cover.cover_code.length] = '\0';
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
        return ThreadResult(.step = FINISH);
    }
    else
        return ThreadResult(.step = ERROR);
}


static ThreadResult 
step_finish(PLC1_Thread4 * self)
{
    Result result = (Result){.DONE = true};
    Execute execute;
    
    if(Cli_DBWrite(self->super.client, PLC1_THREAD4_DB_INDEX, 0, sizeof(Result), &result) != 0
        || Cli_DBRead(self->super.client, PLC1_THREAD4_DB_INDEX, 2, sizeof(Execute), &execute) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);
    else 
        return ThreadResult(.step = FINISH);
        
}


static ThreadResult
step_error(PLC1_Thread4 * self)
{
    Result result = (Result){.ERROR = true};
    Execute execute;
    
    if(Cli_DBWrite(self->super.client, PLC1_THREAD4_DB_INDEX, 0, sizeof(Result), &result) != 0
        || Cli_DBRead(self->super.client, PLC1_THREAD4_DB_INDEX, 2, sizeof(Execute), &execute) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);
    else 
        return ThreadResult(.step = ERROR);
}


static bool
plc1_thread4_run(PLC_Thread * self)
{
    ThreadResult result;

    switch(PLC1_THREAD4(self)->step)
    {
        case WAIT:
            if((result = step_wait(PLC1_THREAD4(self))).is_error == false)
                PLC1_THREAD4(self)->step = result.step;
            else
                return false;

            break;
        case FINISH:
            if((result = step_finish(PLC1_THREAD4(self))).is_error == false)
                PLC1_THREAD4(self)->step = result.step;
            else
                return false;

            break;
        case ERROR:
            if((result = step_error(PLC1_THREAD4(self))).is_error == false)
                PLC1_THREAD4(self)->step = result.step;
            else
                return false;

            break;
        default:
            PLC1_THREAD4(self)->step = WAIT;
    }

    return true;
}


PLC_Thread *
plc1_thread4_new(Model * model, S7Object plc)
{
    PLC1_Thread4 * self = malloc(sizeof(PLC1_Thread4));

    if(self != NULL)
    {
        self->super = PLC_Thread(model, plc, plc1_thread4_run, (void(*)(PLC_Thread*)) free);
        self->step = WAIT;
    }

    return (PLC_Thread*) self;
}



