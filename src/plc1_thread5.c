#include "plc1_thread5.h"
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
    uint8_t execute:1;
}Execute;


typedef struct
{
    uint8_t DONE:1;
    uint8_t VALID:1;
    uint8_t ERROR:1;
    uint8_t REWORK;
}Result;


typedef struct
{
    PLC_String frame_code;
    PLC_String table;
}Frame;


typedef struct
{
    PLC_Thread super;

    ThreadState step;
    uint8_t rework;
    bool valid;
}PLC1_Thread5;


#define PLC1_THREAD5(T)((PLC1_Thread5*)T)


static ThreadResult 
step_wait(PLC1_Thread5 * self)
{
    Execute execute;
    Frame frame;
    Result result = {0};

    if(Cli_DBRead(self->super.client, PLC1_THREAD5_DB_INDEX, 2, sizeof(Execute), &execute) != 0
        || Cli_DBWrite(self->super.client, PLC1_THREAD5_DB_INDEX, 0, sizeof(Result), &result) != 0) 
    {
        return ThreadResult(.is_error = true);
    }
    
    if(execute.execute == false) 
        return ThreadResult(.step = WAIT);

    if(Cli_DBRead(self->super.client, PLC1_THREAD5_DB_INDEX, 4, sizeof(Frame), &frame) != 0)
        return ThreadResult(.is_error = true);

    // set terminating zero character
    frame.frame_code.array[frame.frame_code.length] = '\0';
    frame.table.array[frame.table.length] = '\0';

    TableContent * record = 
        model_get_table_content(
            self->super.model
            , frame.table.array
            , "PA30R_rework"
            , "frame_code"
            , frame.frame_code.array);

    if(record == NULL)
        return ThreadResult(.step = ERROR);

    if(record->super.size == 1)
    {
        self->rework = atoi(record->array[0]->array[0]);
        self->valid = true;
    }
    else
    {
        self->rework = 0;
        self->valid = false;
    }
    
    array_delete(ARRAY(record));

    return ThreadResult(.step = FINISH);
}


static ThreadResult
step_finish(PLC1_Thread5 * self)
{
    Execute execute;
    Result result = {.DONE = true, .ERROR = false, .VALID = self->valid, .REWORK = self->rework};

    if(Cli_DBRead(self->super.client, PLC1_THREAD5_DB_INDEX, 2, sizeof(Execute), &execute) != 0
        || Cli_DBWrite(self->super.client, PLC1_THREAD5_DB_INDEX, 0, sizeof(Result), &result) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);
    else
        return ThreadResult(.step = FINISH);
}


static ThreadResult
step_error(PLC1_Thread5 * self)
{
    Execute execute;
    Result result = {.ERROR = true};

    if(Cli_DBRead(self->super.client, PLC1_THREAD5_DB_INDEX, 2, sizeof(Execute), &execute) != 0
        || Cli_DBWrite(self->super.client, PLC1_THREAD5_DB_INDEX, 0, sizeof(Result), &result) != 0)
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

    switch(PLC1_THREAD5(self)->step)
    {
        case WAIT:
            if((result = step_wait(PLC1_THREAD5(self))).is_error == false)
                PLC1_THREAD5(self)->step = result.step;
            else
                return false;

            break;
        case FINISH:
            if((result = step_finish(PLC1_THREAD5(self))).is_error == false)
                PLC1_THREAD5(self)->step = result.step;
            else
                return false;

            break;
        case ERROR:
            if((result = step_error(PLC1_THREAD5(self))).is_error == false)
                PLC1_THREAD5(self)->step = result.step;
            else
                return false;

            break;
        default:
            PLC1_THREAD5(self)->step = WAIT;
    }

    return true;
}


PLC_Thread *
plc1_thread5_new(Model * model, S7Object client)
{
    PLC1_Thread5 * self = malloc(sizeof(PLC1_Thread5));

    if(self != NULL)
    {
        self->super = PLC_Thread(model, client, thread_run, (void(*)(PLC_Thread*)) free);
        self->step = WAIT;
        self->rework = 0;
        self->valid = false;
    }

    return PLC_THREAD(self);
}
