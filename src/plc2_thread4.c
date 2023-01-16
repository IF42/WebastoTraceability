#include "plc2_thread4.h"

#include <stdlib.h>
#include <string.h>

#define DB_INDEX 22

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
    uint8_t execute:1;
}Execute;


typedef struct 
{
    uint8_t DONE:1;
    uint8_t ERROR:1;
    uint8_t PA60R_PROCESSED:1;
    DTL PA30R_PROCESS_DATE_TIME;
}Result;


typedef struct 
{
    PLC_Thread super;

    ThreadStep step;

    bool pa60r_processed;
    DTL pa30r_process_date_time;
}PLC2_Thread4;


#define PLC2_Thread4(...)(PLC2_Thread4) {__VA_ARGS__}


#define PLC2_THREAD4(T)((PLC2_Thread4*) T)



static ThreadResult
step_wait(PLC2_Thread4 * self)
{
    Result result = {0};
    PLC_String frame_code;
    Execute execute;

    if(Cli_DBWrite(self->super.client, DB_INDEX, 0, sizeof(Result), &result) != 0
        || Cli_DBRead(self->super.client, DB_INDEX, 14, sizeof(Execute), &execute) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);

    if(Cli_DBRead(self->super.client, DB_INDEX, 16, sizeof(PLC_String), &frame_code) != 0)
        return ThreadResult(.is_error = true);

    frame_code.array[frame_code.length] = '\0';

    TableContent * record = 
         model_get_table_content(
            self->super.model
            , "frame_582"
            , "(PA30R_date_time, PA60R_cyclus_completed)"
            , "frame_code"
            , frame_code.array);

    if(record != NULL)
    {
        if(record->super.size == 1)
        {
            sscanf(
                record->array[0]->array[0]
                , "%hhd.%hhd.%hd*%hhd:%hhd"
                , (signed char*) &self->pa30r_process_date_time.day
                , (signed char*) &self->pa30r_process_date_time.month
                , (short int*) &self->pa30r_process_date_time.year
                , (signed char*) &self->pa30r_process_date_time.hour
                , (signed char*)&self->pa30r_process_date_time.minute);

            if(strcmp(record->array[0]->array[1], "*") != 0)
                self->pa60r_processed = true;
            else
                self->pa60r_processed = false;

            array_delete(ARRAY(record));

            return ThreadResult(.step = FINISH);
        }
        else
        {
            array_delete(ARRAY(record));
            return ThreadResult(.step = ERROR);
        }
    }
    else
        return ThreadResult(.step = ERROR);
}


static ThreadResult
step_finish(PLC2_Thread4 * self)
{
    Result result = 
        {
            .DONE = true
            , .ERROR = false
            , .PA30R_PROCESS_DATE_TIME = self->pa30r_process_date_time
            , .PA60R_PROCESSED = self->pa60r_processed
        };
    Execute execute;
    
    if(Cli_DBWrite(self->super.client, DB_INDEX, 0, sizeof(Result), &result) != 0
        || Cli_DBRead(self->super.client, DB_INDEX, 14, sizeof(execute), &execute) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);
    else
        return ThreadResult(.step = FINISH);
}

static ThreadResult
step_error(PLC2_Thread4 * self)
{
    Result result = {.ERROR = false };
    Execute execute;
    
    if(Cli_DBWrite(self->super.client, DB_INDEX, 0, sizeof(Result), &result) != 0
        || Cli_DBRead(self->super.client, DB_INDEX, 14, sizeof(execute), &execute) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);
    else
        return ThreadResult(.step = ERROR);
}


static bool
plc2_thread4_run(PLC_Thread * self)
{
    ThreadResult result;

    switch(PLC2_THREAD4(self)->step)
    {
        case WAIT:
            if((result = step_wait(PLC2_THREAD4(self))).is_error == false)
                PLC2_THREAD4(self)->step = result.step;
            else    
                return false;

            break;
        case FINISH:
            if((result = step_finish(PLC2_THREAD4(self))).is_error == false)
                PLC2_THREAD4(self)->step = result.step;
            else
                return false;

            break;
        case ERROR:
            if((result = step_error(PLC2_THREAD4(self))).is_error == false)
                PLC2_THREAD4(self)->step = result.step;
            else
                return false;

            break;
        default:
            PLC2_THREAD4(self)->step = WAIT;
    }

    return true;
}


PLC_Thread *
plc2_thread4_new(
    Model * model
    , S7Object client)
{
    PLC2_Thread4 * self = malloc(sizeof(PLC2_Thread4));

    if(self != NULL)
    {
        self->super = PLC_Thread(model, client, plc2_thread4_run, (void(*)(PLC_Thread*)) free);
        self->step = WAIT;
    }

    return PLC_THREAD(self);
}


