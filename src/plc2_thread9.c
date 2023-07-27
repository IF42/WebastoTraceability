#include "plc2_thread9.h"
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
}PLC2_Thread9;


#define PLC2_Thread9(...)(PLC2_Thread9) {__VA_ARGS__}

#define PLC2_THREAD9(T)((PLC2_Thread9*) T)

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
    PLC_String MATERIAL_DESCRIPTION;
    PLC_String MATERIAL_TYPE;
    PLC_String BATCH_CODE;
    PLC_String VALIDITY;
}Glue;


static ThreadResult
step_wait(PLC2_Thread9 * self)
{
    Execute execute;
    Glue glue;
    Result result = {0};

    if(Cli_DBRead(self->super.client, PLC2_THREAD9_DB_INDEX, 2, sizeof(Execute), &execute) != 0
        || Cli_DBWrite(self->super.client, PLC2_THREAD9_DB_INDEX, 0, sizeof(Result), &result) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);

    if(Cli_DBRead(self->super.client, PLC2_THREAD9_DB_INDEX, 4, sizeof(Glue), &glue) != 0)
        return ThreadResult(.is_error = true);

    glue.MATERIAL_DESCRIPTION.array[glue.MATERIAL_DESCRIPTION.length] = '\0';
    glue.MATERIAL_TYPE.array[glue.MATERIAL_TYPE.length]               = '\0';
    glue.BATCH_CODE.array[glue.BATCH_CODE.length]                     = '\0';
    glue.VALIDITY.array[glue.VALIDITY.length]                         = '\0';

    if(model_write_glue_barrel(
        self->super.model
        , glue.MATERIAL_DESCRIPTION.array
        , glue.MATERIAL_TYPE.array
        , glue.BATCH_CODE.array
        , glue.VALIDITY.array) == true)
    {
        return ThreadResult(.step = FINISH);
    }
    else
        return ThreadResult(.step = ERROR);
}


static ThreadResult
step_finish(PLC2_Thread9 * self)
{
    Result result = {.DONE = true};
    Execute execute;
    
    if(Cli_DBRead(self->super.client, PLC2_THREAD9_DB_INDEX, 2, sizeof(Execute), &execute) != 0
        || Cli_DBWrite(self->super.client, PLC2_THREAD9_DB_INDEX, 0, sizeof(Result), &result) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);
    else
        return ThreadResult(.step = FINISH);
}


static ThreadResult
step_error(PLC2_Thread9 * self)
{
    Result result = {.ERROR = true};
    Execute execute;
    
    if(Cli_DBRead(self->super.client, PLC2_THREAD9_DB_INDEX, 2, sizeof(Execute), &execute) != 0
        || Cli_DBWrite(self->super.client, PLC2_THREAD9_DB_INDEX, 0, sizeof(Result), &result) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);
    else
        return ThreadResult(.step = ERROR);
}


static bool
plc2_thread9_run(PLC_Thread * self)
{
    ThreadResult result;

    switch(PLC2_THREAD9(self)->step)
    {
        case WAIT:
            if((result = step_wait(PLC2_THREAD9(self))).is_error == false)
                PLC2_THREAD9(self)->step = result.step;
            else    
                return false;

            break;
        case FINISH:
            if((result = step_finish(PLC2_THREAD9(self))).is_error == false)
                PLC2_THREAD9(self)->step = result.step;
            else
                return false;

            break;
        case ERROR:
            if((result = step_error(PLC2_THREAD9(self))).is_error == false)
                PLC2_THREAD9(self)->step = result.step;
            else
                return false;

            break;
        default:
            PLC2_THREAD9(self)->step = WAIT;
    }

    return true;
}


PLC_Thread *
plc2_thread9_new(
    Model * model
    , S7Object client)
{
    PLC2_Thread9 * self = malloc(sizeof(PLC2_Thread9));

    if(self != NULL)
    {
        self->super = PLC_Thread(model, client, plc2_thread9_run, (void(*)(PLC_Thread*)) free);
        self->step = WAIT;
    }

    return PLC_THREAD(self);
}
