#include "plc1_thread11.h"
#include "plc_thread_config.h"

#include <util.h>
#include <stdlib.h>
#include <string.h>


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

    char * csv_path;
}PLC1_Thread11;

#define PLC1_THREAD11(T)((PLC1_Thread11*)T)


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
    int16_t temperature;
    int16_t humidity;
}Frame;


static ThreadResult 
step_wait(PLC1_Thread11 * self)
{
    Execute execute;
    Frame frame;
    Result result = {0};

    if(Cli_DBRead(self->super.client, PLC1_THREAD11_DB_INDEX, 2, sizeof(Execute), &execute) != 0
        || Cli_DBWrite(self->super.client, PLC1_THREAD11_DB_INDEX, 0, sizeof(Result), &result) != 0)
    {
        return ThreadResult(.is_error = true);
    } 

    if(execute.execute == false) 
        return ThreadResult(.step = WAIT);

    if(Cli_DBRead(self->super.client, PLC1_THREAD11_DB_INDEX, 4, sizeof(Frame), &frame) != 0)
        return ThreadResult(.is_error = true);

    frame.table.array[frame.table.length]           = '\0';
    frame.frame_code.array[frame.frame_code.length] = '\0';

    if(model_generate_frame_csv(
        self->super.model
        , "."
        , frame.table.array
        , frame.frame_code.array
        , ((float)frame.temperature)/10.0
        , ((float)frame.humidity)/10.0) == true)
    {
        return ThreadResult(.step = FINISH);
    }
    else
        return ThreadResult(.step = ERROR);
}


static ThreadResult
step_finish(PLC1_Thread11 * self)
{
    Execute execute;
    Result result = {.DONE = true};
    
    if(Cli_DBRead(self->super.client, PLC1_THREAD11_DB_INDEX, 2, sizeof(Execute), &execute) != 0
        || Cli_DBWrite(self->super.client, PLC1_THREAD11_DB_INDEX, 0, sizeof(Result), &result) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);
    else
        return ThreadResult(.step = FINISH);
}


static ThreadResult 
step_error(PLC1_Thread11 * self)
{
    Execute execute;
    Result result = {.ERROR = true};

    if(Cli_DBRead(self->super.client, PLC1_THREAD11_DB_INDEX, 2, sizeof(Execute), &execute) != 0
        || Cli_DBWrite(self->super.client, PLC1_THREAD11_DB_INDEX, 0, sizeof(Result), &result) != 0)
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

    switch(PLC1_THREAD11(self)->step)
    {
        case WAIT:
            if((result = step_wait(PLC1_THREAD11(self))).is_error == false)
                PLC1_THREAD11(self)->step = result.step;
            else
                return false;

            break;
        case FINISH:
            if((result = step_finish(PLC1_THREAD11(self))).is_error == false)
                PLC1_THREAD11(self)->step = result.step;
            else
                return false;

            break;
        case ERROR:
            if((result = step_error(PLC1_THREAD11(self))).is_error == false)
                PLC1_THREAD11(self)->step = result.step;
            else
                return false;

            break;
        default:
            PLC1_THREAD11(self)->step = WAIT;
    }

    return true;
}


PLC_Thread *
plc1_thread11_new(
    Model * model
    , S7Object client
    , char * csv_path)
{
    PLC1_Thread11 * self = malloc(sizeof(PLC1_Thread11));

    if(self != NULL)
    {
        self->super = PLC_Thread(model, client, thread_run, (void(*)(PLC_Thread*)) free);
        self->step = WAIT;

        self->csv_path = strdup(csv_path);
    }

    return PLC_THREAD(self);
}



