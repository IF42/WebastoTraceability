#include "plc1_thread6.h"
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
}PLC1_Thread6;


#define PLC1_THREAD6(T)((PLC1_Thread6*) T)

typedef struct
{
    uint8_t DONE:1;
    uint8_t ERROR:1;
}Result;


typedef struct
{
    uint8_t execute:1;
}Execute;


typedef struct
{
    uint8_t rework;
    PLC_String table;
    PLC_String frame_code;
}Rework;


static ThreadResult
step_wait(PLC1_Thread6 * self)
{
    Execute execute;
    Rework rework;
    Result result = {0};

    if(Cli_DBRead(self->super.client, PLC1_THREAD6_DB_INDEX, 2, sizeof(Execute), &execute) != 0
        || Cli_DBWrite(self->super.client, PLC1_THREAD6_DB_INDEX, 0, sizeof(Result), &result) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false) return ThreadResult(.step = WAIT);

    if(Cli_DBRead(self->super.client, PLC1_THREAD6_DB_INDEX, 3, sizeof(Rework), &rework) != 0)
        return ThreadResult(.is_error = true);

    rework.table.array[rework.table.length] = '\0';
    rework.frame_code.array[rework.frame_code.length] = '\0';

    if(model_pa30r_update_frame_rework(
        self->super.model
        , rework.table.array
        , rework.frame_code.array
        , rework.rework) == true)
    {
        return ThreadResult(.step = FINISH);
    }
    else
        return ThreadResult(.step = ERROR); 
}


static ThreadResult
step_finish(PLC1_Thread6 * self)
{
    Result result = {.DONE = true};
    Execute execute;

    if(Cli_DBRead(self->super.client, PLC1_THREAD6_DB_INDEX, 2, sizeof(Execute), &execute) != 0
         || Cli_DBWrite(self->super.client, PLC1_THREAD6_DB_INDEX, 0, sizeof(Result), &result) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);
    else
        return ThreadResult(.step = FINISH);
}

static ThreadResult
step_error(PLC1_Thread6 * self)
{
    Result result = {.ERROR = true};
    Execute execute;

    if(Cli_DBRead(self->super.client, PLC1_THREAD6_DB_INDEX, 2, sizeof(Execute), &execute) != 0
         || Cli_DBWrite(self->super.client, PLC1_THREAD6_DB_INDEX, 0, sizeof(Result), &result) != 0)
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

    switch(PLC1_THREAD6(self)->step)
    {
        case WAIT:
            if((result = step_wait(PLC1_THREAD6(self))).is_error == false)
                PLC1_THREAD6(self)->step = result.step;
            else
                return false;
            
            break;
        case FINISH:
            if((result = step_finish(PLC1_THREAD6(self))).is_error == false)
                PLC1_THREAD6(self)->step = result.step;
            else
                return false;

            break;
        case ERROR:
                if((result = step_error(PLC1_THREAD6(self))).is_error == false)
                    PLC1_THREAD6(self)->step = result.step;
                else
                    return false;
            break;
        default:
            PLC1_THREAD6(self)->step = WAIT;
    }
    
    return true;
}


PLC_Thread *
plc1_thread6_new(
    Model * model
    , S7Object client)
{
    PLC1_Thread6 * self = malloc(sizeof(PLC1_Thread6));
    
    if(self != NULL)
    {
        self->super = PLC_Thread(model, client, thread_run, (void(*)(PLC_Thread*)) free);
        self->step = WAIT;
    }

    return PLC_THREAD(self);
}



