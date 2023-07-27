#include "plc2_thread5.h"
#include "plc_thread_config.h"

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

    bool SPARE_PART;
    bool USED;
    PLC_String ORIENTATION;
    DTL PA10B_PROCESS_TIME;
}PLC2_Thread5;

#define PLC2_THREAD5(T)((PLC2_Thread5*) T)


typedef struct
{
    uint8_t execute:1;
}Execute;


typedef struct
{
    uint8_t DONE:1;
    uint8_t ERROR:1;
    uint8_t SPARE_PART:1;
    uint8_t USED:1;
    uint8_t unreachable;
    PLC_String ORIENTATION;
    DTL PA10B_PROCESS_TIME;
}Result;


static ThreadResult
step_wait(PLC2_Thread5 * self)
{
    Execute execute;
    PLC_String cover_code;
    TableContent * record;
    Result result = 
        {
            .ORIENTATION          = {.size=254}
            , .PA10B_PROCESS_TIME = DTL_DEFAULT
        };

    if(Cli_DBRead(self->super.client, PLC2_THREAD5_DB_INDEX, 270, sizeof(Execute), &execute) != 0 
        || Cli_DBWrite(self->super.client, PLC2_THREAD5_DB_INDEX, 0, sizeof(Result), &result) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);

    if(Cli_DBRead(self->super.client, PLC2_THREAD5_DB_INDEX, 272, sizeof(PLC_String), &cover_code) != 0)
        return ThreadResult(.is_error = true);

    cover_code.array[cover_code.length] = '\0';

    if((record = model_read_cover(self->super.model, cover_code.array)) != NULL 
        && record->super.size == 1)
    {
        strncpy(self->ORIENTATION.array, record->array[0]->array[0], 253);
        self->ORIENTATION.size = 254;
        self->ORIENTATION.length = strlen(record->array[0]->array[0]);
        
        sscanf(
            record->array[0]->array[1]
            , "%hhd.%hhd.%hd*%hhd:%hhd"
            , (signed char*) &self->PA10B_PROCESS_TIME.day
            , (signed char*) &self->PA10B_PROCESS_TIME.month
            , (short int*) &self->PA10B_PROCESS_TIME.year
            , (signed char*) &self->PA10B_PROCESS_TIME.hour
            , (signed char*)&self->PA10B_PROCESS_TIME.minute);

        self->PA10B_PROCESS_TIME.year       = swap_endian(self->PA10B_PROCESS_TIME.year);
        self->PA10B_PROCESS_TIME.nanosecond = 0;

        if(strcmp(record->array[0]->array[2], "YES") == 0)
            self->USED = true;
        else
            self->USED = false;

        if(strcmp(record->array[0]->array[3], "YES") == 0)
            self->SPARE_PART = true;
        else
            self->SPARE_PART = false;

        array_delete(ARRAY(record));
        return ThreadResult(.step = FINISH);
    }
    else if(record != NULL && record->super.size > 1)
    {
        array_delete(ARRAY(record));
        return ThreadResult(.step = ERROR);
    }
    else
        return ThreadResult(.step = ERROR);
}   


static ThreadResult
step_finish(PLC2_Thread5 * self)
{
    Execute execute;
    Result result = 
        {
            .DONE = true
            , .SPARE_PART = self->SPARE_PART
            , .USED = self->USED
            , .ORIENTATION = self->ORIENTATION
            , .PA10B_PROCESS_TIME = self->PA10B_PROCESS_TIME
        };

     if(Cli_DBRead(self->super.client, PLC2_THREAD5_DB_INDEX, 270, sizeof(Execute), &execute) != 0
        || Cli_DBWrite(self->super.client, PLC2_THREAD5_DB_INDEX, 0, sizeof(Result), &result) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);
    else
        return ThreadResult(.step = FINISH);
}


static ThreadResult 
step_error(PLC2_Thread5 * self)
{
    Execute execute;
    Result result = 
        {
            .ERROR = true
            , .PA10B_PROCESS_TIME = DTL_DEFAULT
            , .ORIENTATION = {.size = 254}
        };

    if(Cli_DBRead(self->super.client, PLC2_THREAD5_DB_INDEX, 270, sizeof(Execute), &execute) != 0
        || Cli_DBWrite(self->super.client, PLC2_THREAD5_DB_INDEX, 0, sizeof(Result), &result) != 0)
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

    switch(PLC2_THREAD5(self)->step)
    {
        case WAIT:
            if((result = step_wait(PLC2_THREAD5(self))).is_error == false)
                PLC2_THREAD5(self)->step = result.step;
            else
                return false;

            break;
        case FINISH:
            if((result = step_finish(PLC2_THREAD5(self))).is_error == false)
                PLC2_THREAD5(self)->step = result.step;
            else
                return false;

            break;
        case ERROR:
            if((result = step_error(PLC2_THREAD5(self))).is_error == false)
                PLC2_THREAD5(self)->step = result.step;
            else
                return false;

            break;
        default:
            PLC2_THREAD5(self)->step = WAIT;
    }

    return true;
}


PLC_Thread *
plc2_thread5_new(
    Model * model
    , S7Object client)
{
    PLC2_Thread5 * self = malloc(sizeof(PLC2_Thread5));

    if(self != NULL)
    {
        self->super = PLC_Thread(model, client, thread_run, (void(*)(PLC_Thread*)) free);
        self->step = WAIT;

        self->USED               = false;
        self->SPARE_PART         = false;
        self->ORIENTATION        = (PLC_String){.size=254, .length = 0};
        self->PA10B_PROCESS_TIME = DTL_DEFAULT;
    }

    return PLC_THREAD(self);
}
