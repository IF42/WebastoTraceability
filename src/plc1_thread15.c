#include "plc1_thread15.h"
#include "plc_thread_config.h"

#include <stdlib.h>


typedef enum
{
    WAIT
    , FINISH
    , ERROR
}ThreadState;

typedef struct
{
    bool is_error;
    ThreadState step;
}ThreadResult;

#define ThreadResult(...)(ThreadResult){__VA_ARGS__}


typedef struct
{
    PLC_Thread super;
    ThreadState step;
}PLC1_Thread15;

#define PLC1_THREAD15(T)((PLC1_Thread15*) T)

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
    PLC_String batch_code;
    uint8_t fill_counter;
}Bottle;


static ThreadResult
step_wait(PLC1_Thread15 * self)
{
    Execute execute;
    Result result = {0};
    Bottle bottle;

    if(Cli_DBRead(self->super.client, PLC1_THREAD15_DB_INDEX, 2, sizeof(Execute), &execute) != 0
        || Cli_DBWrite(self->super.client, PLC1_THREAD15_DB_INDEX, 0, sizeof(Result), &result) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);

    if(Cli_DBRead(self->super.client, PLC1_THREAD15_DB_INDEX, 4, sizeof(Bottle), &bottle) != 0)
        return ThreadResult(.is_error = true);
    
    bottle.batch_code.array[bottle.batch_code.length] = '\0';
    
    if(model_update_bottle_fill_counter(
        self->super.model
        , bottle.batch_code.array
        , bottle.fill_counter) == true)
    {
        return ThreadResult(.step = FINISH);
    }
    else
        return ThreadResult(.step = ERROR);
}


static ThreadResult
step_finish(PLC1_Thread15 * self)
{
    Execute execute;
    Result result = {.DONE = true};
    
    if(Cli_DBRead(self->super.client, PLC1_THREAD15_DB_INDEX, 2, sizeof(Execute), &execute) != 0
        || Cli_DBWrite(self->super.client, PLC1_THREAD15_DB_INDEX, 0, sizeof(Result), &result) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);
    else
        return ThreadResult(.step = FINISH);
}


static ThreadResult 
step_error(PLC1_Thread15 * self)
{
    Execute execute;
    Result result = {.ERROR = true};
    
    if(Cli_DBRead(self->super.client, PLC1_THREAD15_DB_INDEX, 2, sizeof(Execute), &execute) != 0
        || Cli_DBWrite(self->super.client, PLC1_THREAD15_DB_INDEX, 0, sizeof(Result), &result) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);
    else
        return ThreadResult(.step = ERROR);
}


static bool
thread_run(PLC_Thread * self)
{
    ThreadResult result;

    switch(PLC1_THREAD15(self)->step)
    {
        case WAIT:
            if((result = step_wait(PLC1_THREAD15(self))).is_error == false)
                PLC1_THREAD15(self)->step = result.step;
            else
                return false;

            break;
        case FINISH:
            if((result = step_finish(PLC1_THREAD15(self))).is_error == false)
                PLC1_THREAD15(self)->step = result.step;
            else
                return false;

            break;
        case ERROR:
            if((result = step_error(PLC1_THREAD15(self))).is_error == false)
                PLC1_THREAD15(self)->step = result.step;
            else
                return false;

            break;
        default:
            PLC1_THREAD15(self)->step = WAIT;
    }

    return true;
}


PLC_Thread *
plc1_thread15_new(
    Model * model
    , S7Object client)
{
    PLC1_Thread15 * self = malloc(sizeof(PLC1_Thread15));

    if(self != NULL)
    {
        self->super = PLC_Thread(model, client, thread_run, (void(*)(PLC_Thread*)) free);
        self->step = WAIT;
    }

    return PLC_THREAD(self);
}
