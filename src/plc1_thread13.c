#include "plc1_thread13.h"
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
}PLC1_Thread13;

#define PLC1_THREAD13(T)((PLC1_Thread13*) T)


typedef struct
{
    uint8_t execute:1;
}Execute;


typedef struct
{
    PLC_String chemical_type;
    PLC_String batch_code;
}Bottle;

typedef struct
{
    uint8_t DONE:1;
    uint8_t ERROR:1;
}Result;


static ThreadResult
step_wait(PLC1_Thread13 * self)
{
    Execute execute;
    Bottle bottle;
    Result result = {0};

    if(Cli_DBRead(self->super.client, PLC1_THREAD13_DB_INDEX, 2, sizeof(Execute), &execute) != 0
        || Cli_DBWrite(self->super.client, PLC1_THREAD13_DB_INDEX, 0, sizeof(Result), &result) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);

    if(Cli_DBRead(self->super.client, PLC1_THREAD13_DB_INDEX, 4, sizeof(Bottle), &bottle) != 0)
        return ThreadResult(.is_error = true);

    bottle.chemical_type.array[bottle.chemical_type.length] = '\0';
    bottle.batch_code.array[bottle.batch_code.length] = '\0';

    if(model_write_supplier_bottle(
        self->super.model
        , bottle.chemical_type.array
        , bottle.batch_code.array) == true)
    {
        return ThreadResult(.step = FINISH);
    }
    else
        return ThreadResult(.step = ERROR);
}


static ThreadResult
step_finish(PLC1_Thread13 * self)
{
    Result result = {.DONE = true};
    Execute execute;

    if(Cli_DBRead(self->super.client, PLC1_THREAD13_DB_INDEX, 2, sizeof(Execute), &execute) != 0
        || Cli_DBWrite(self->super.client, PLC1_THREAD13_DB_INDEX, 0, sizeof(Result), &result) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);
    else
        return ThreadResult(.step = FINISH);
}


static ThreadResult 
step_error(PLC1_Thread13 * self)
{
    Result result = {.ERROR = true};
    Execute execute;

    if(Cli_DBRead(self->super.client, PLC1_THREAD13_DB_INDEX, 2, sizeof(Execute), &execute) != 0
        || Cli_DBWrite(self->super.client, PLC1_THREAD13_DB_INDEX, 0, sizeof(Result), &result) != 0)
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

    switch(PLC1_THREAD13(self)->step)
    {
        case WAIT:
            if((result = step_wait(PLC1_THREAD13(self))).is_error == false)
                PLC1_THREAD13(self)->step = result.step;
            else
                return false;

            break;
        case FINISH:
            if((result = step_finish(PLC1_THREAD13(self))).is_error == false)
                PLC1_THREAD13(self)->step = result.step;
            else
                return false;

            break;
        case ERROR:
            if((result = step_error(PLC1_THREAD13(self))).is_error == false)
                PLC1_THREAD13(self)->step = result.step;
            else
                return false;

            break;
        default:
            PLC1_THREAD13(self)->step = WAIT;
    }

    return true;
}


PLC_Thread *
plc1_thread13_new(
    Model * model
    , S7Object client)
{
    PLC1_Thread13 * self = malloc(sizeof(PLC1_Thread13));

    if(self != NULL)
    {
        self->super = PLC_Thread(model, client, thread_run, (void(*)(PLC_Thread*)) free);
        self->step = WAIT;
    }

    return PLC_THREAD(self);
}
