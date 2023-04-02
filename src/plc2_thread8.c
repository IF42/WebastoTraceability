#include "plc2_thread8.h"
#include "plc_thread_config.h"

#include <stdlib.h>
#include <string.h>


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
    PLC_Thread super;
    ThreadStep step;
}PLC2_Thread8;

#define PLC2_Thread8(...)(PLC2_Thread8) {__VA_ARGS__}

#define PLC2_THREAD8(T)((PLC2_Thread8*) T)


typedef struct
{
    uint8_t execute:1;
}Execute;


typedef struct
{
    uint8_t DONE:1;
    uint8_t ERROR:1;
}Result;


static ThreadResult
step_wait(PLC2_Thread8 * self)
{
    Execute execute;
    PLC_String cover_code;
    Result result = {0};

    if(Cli_DBRead(self->super.client, PLC2_THREAD8_DB_INDEX, 2, sizeof(Execute), &execute) != 0
        || Cli_DBWrite(self->super.client, PLC2_THREAD8_DB_INDEX, 0, sizeof(Result), &result) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);

    if(Cli_DBRead(self->super.client, PLC2_THREAD8_DB_INDEX, 4, sizeof(PLC_String), &cover_code) != 0)
        return ThreadResult(.is_error = true);

    cover_code.array[cover_code.length] = '\0';
    
    if(model_set_cover_used(self->super.model, cover_code.array) == true)
        return ThreadResult(.step = FINISH);
    else
        return ThreadResult(.step = ERROR);
}


static ThreadResult
step_finish(PLC2_Thread8 * self)
{
    Execute execute;
    Result result = {.DONE = true};

    if(Cli_DBRead(self->super.client, PLC2_THREAD8_DB_INDEX, 2, sizeof(Execute), &execute) != 0
        || Cli_DBWrite(self->super.client, PLC2_THREAD8_DB_INDEX, 0, sizeof(Result), &result) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);
    else
        return ThreadResult(.step = FINISH);
}


static ThreadResult
step_error(PLC2_Thread8 * self)
{
    Execute execute;
    Result result = {.ERROR = true};

    if(Cli_DBRead(self->super.client, PLC2_THREAD8_DB_INDEX, 2, sizeof(Execute), &execute) != 0
        || Cli_DBWrite(self->super.client, PLC2_THREAD8_DB_INDEX, 0, sizeof(Result), &result) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);
    else
        return ThreadResult(.step = ERROR);
}


static bool
plc2_thread8_run(PLC_Thread * self)
{
    ThreadResult result;

    switch(PLC2_THREAD8(self)->step)
    {
        case WAIT:
            if((result = step_wait(PLC2_THREAD8(self))).is_error == false)
                PLC2_THREAD8(self)->step = result.step;
            else    
                return false;

            break;
        case FINISH:
            if((result = step_finish(PLC2_THREAD8(self))).is_error == false)
                PLC2_THREAD8(self)->step = result.step;
            else
                return false;

            break;
        case ERROR:
            if((result = step_error(PLC2_THREAD8(self))).is_error == false)
                PLC2_THREAD8(self)->step = result.step;
            else
                return false;

            break;
        default:
            PLC2_THREAD8(self)->step = WAIT;
    }

    return true;
}


PLC_Thread *
plc2_thread8_new(
    Model * model
    , S7Object client)
{
    PLC2_Thread8 * self = malloc(sizeof(PLC2_Thread8));

    if(self != NULL)
    {
        self->super = PLC_Thread(model, client, plc2_thread8_run, (void(*)(PLC_Thread*)) free);
        self->step  = WAIT;
    }

    return PLC_THREAD(self);
}
