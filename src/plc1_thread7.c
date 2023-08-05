#include "plc1_thread7.h"
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
    uint8_t ERROR:1;
}Result;


typedef struct
{
    PLC_String table;
    PLC_String frame_code;
    PLC_String primer_207_sika;
    PLC_String remover_208_sika;
    uint32_t order;
}Frame;


typedef struct
{
    PLC_Thread super;
    ThreadState step;
}PLC1_Thread7;


#define PLC1_THREAD7(T)((PLC1_Thread7*) T)


static ThreadResult
step_wait(PLC1_Thread7 * self)
{
    Execute execute;
    Frame frame;
    Result result = {0};

    if(Cli_DBRead(self->super.client, PLC1_THREAD7_DB_INDEX, 2, sizeof(Execute), &execute) != 0
        || Cli_DBWrite(self->super.client, PLC1_THREAD7_DB_INDEX, 0, sizeof(Result), &result) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false) 
        return ThreadResult(.step = WAIT);

    if(Cli_DBRead(self->super.client, PLC1_THREAD7_DB_INDEX, 4, sizeof(Frame), &frame) != 0)
        return ThreadResult(.is_error = true);
    
    frame.table.array[frame.table.length] = '\0';
    frame.frame_code.array[frame.frame_code.length] = '\0';
    frame.primer_207_sika.array[frame.primer_207_sika.length] = '\0';
    frame.remover_208_sika.array[frame.remover_208_sika.length] = '\0';

    if(model_write_new_frame(
        self->super.model
        , frame.table.array
        , frame.frame_code.array
        , swap_endian(frame.order)
        , frame.primer_207_sika.array
        , frame.remover_208_sika.array) == true)
    {
        return ThreadResult(.step = FINISH);
    }
    else
        return ThreadResult(.step = ERROR);
}


static ThreadResult
step_finish(PLC1_Thread7 * self)
{
    Execute execute;
    Result result = (Result) {.DONE = true};

    if(Cli_DBWrite(self->super.client, PLC1_THREAD7_DB_INDEX, 0, sizeof(Result), &result) != 0
        || Cli_DBRead(self->super.client, PLC1_THREAD7_DB_INDEX, 2, sizeof(Execute), &execute) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);
    else
        return ThreadResult(.step = FINISH);
}


static ThreadResult
step_error(PLC1_Thread7 * self)
{
    Execute execute;
    Result result = (Result) {.ERROR = true};

    if(Cli_DBWrite(self->super.client, PLC1_THREAD7_DB_INDEX, 0, sizeof(Result), &result) != 0
        || Cli_DBRead(self->super.client, PLC1_THREAD7_DB_INDEX, 2, sizeof(Execute), &execute) != 0)
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

    switch(PLC1_THREAD7(self)->step)
    {
        case WAIT:
            if((result = step_wait(PLC1_THREAD7(self))).is_error == false)
                PLC1_THREAD7(self)->step = result.step;
            else
                return false;

            break;
        case FINISH:
            if((result = step_finish(PLC1_THREAD7(self))).is_error == false)    
                PLC1_THREAD7(self)->step = result.step;
            else
                return false;

            break;
        case ERROR:
            if((result = step_error(PLC1_THREAD7(self))).is_error == false)
                PLC1_THREAD7(self)->step = result.step;
            else
                return false;

            break;
        default:
            PLC1_THREAD7(self)->step = WAIT;
    }

    return true;
}


PLC_Thread *
plc1_thread7_new(
    Model * model
    , S7Object client)
{
    PLC1_Thread7 * self = malloc(sizeof(PLC1_Thread7));

    if(self != NULL)
    {
        self->super = PLC_Thread(model, client, thread_run, (void(*)(PLC_Thread *)) free);
        self->step = WAIT;
    }

    return PLC_THREAD(self);
}





