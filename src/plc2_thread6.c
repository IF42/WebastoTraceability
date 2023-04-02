#include "plc2_thread6.h"
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
}PLC2_Thread6;

#define PLC2_THREAD6(T)((PLC2_Thread6*) T)

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
    PLC_String frame_code;
    PLC_String left_cover_code;
    PLC_String right_cover_code;
}Frame;


static ThreadResult
step_wait(PLC2_Thread6 * self)
{
    Execute execute;
    Frame frame;  
    Result result = {0};

    if(Cli_DBRead(self->super.client, PLC2_THREAD6_DB_INDEX, 2, sizeof(Execute), &execute) != 0
        || Cli_DBWrite(self->super.client, PLC2_THREAD6_DB_INDEX, 0, sizeof(Result), &result) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);

    if(Cli_DBRead(self->super.client, PLC2_THREAD6_DB_INDEX, 4, sizeof(Frame), &frame) != 0)
        return ThreadResult(.is_error = true);

    frame.frame_code.array[frame.frame_code.length]             = '\0';
    frame.left_cover_code.array[frame.left_cover_code.length]   = '\0';
    frame.right_cover_code.array[frame.right_cover_code.length] = '\0';

    if(model_pa30b_update_frame(
         self->super.model
        , frame.frame_code.array
        , frame.left_cover_code.array
        , frame.right_cover_code.array) == true)
    {
        return ThreadResult(.step = FINISH);
    }
    else
        return ThreadResult(.step = ERROR);
}

static ThreadResult 
step_finish(PLC2_Thread6 * self)
{
    Execute execute;
    Result result = {.DONE = true};
    
    if(Cli_DBRead(self->super.client, PLC2_THREAD6_DB_INDEX, 2, sizeof(Execute), &execute) != 0
        || Cli_DBWrite(self->super.client, PLC2_THREAD6_DB_INDEX, 0, sizeof(Result), &result) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);
    else
        return ThreadResult(.step = FINISH);
}


static ThreadResult
step_error(PLC2_Thread6 * self)
{
    Execute execute;
    Result result = {.ERROR = true};
    
    if(Cli_DBRead(self->super.client, PLC2_THREAD6_DB_INDEX, 2, sizeof(Execute), &execute) != 0
        || Cli_DBWrite(self->super.client, PLC2_THREAD6_DB_INDEX, 0, sizeof(Result), &result) != 0)
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

    switch(PLC2_THREAD6(self)->step)
    {
        case WAIT:
            if((result = step_wait(PLC2_THREAD6(self))).is_error == false)
                PLC2_THREAD6(self)->step = result.step;
            else
                return false;

            break;
        case FINISH:
            if((result = step_finish(PLC2_THREAD6(self))).is_error == false)
                PLC2_THREAD6(self)->step = result.step;
            else
                return false;

            break;
        case ERROR:
            if((result = step_error(PLC2_THREAD6(self))).is_error == false)
                PLC2_THREAD6(self)->step = result.step;
            else
                return false;

            break;
        default:
            PLC2_THREAD6(self)->step = WAIT;
    }

    return true;
}


PLC_Thread *
plc2_thread6_new(
    Model * model
    , S7Object client)
{
    PLC2_Thread6 * self = malloc(sizeof(PLC2_Thread6));

    if(self != NULL)
    {
        self->super = PLC_Thread(model, client, thread_run, (void(*)(PLC_Thread*)) free);
        self->step = WAIT;
    }

    return PLC_THREAD(self);
}



