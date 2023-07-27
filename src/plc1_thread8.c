#include "plc1_thread8.h"
#include "plc_thread_config.h"

#include <string.h>
#include <stdlib.h>


typedef enum
{
    WAIT
    , FINISH_TRUE
    , FINISH_FALSE
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

    bool    VALID;
    bool    PA60R_PROCESSED;
    uint8_t REWORK;
    DTL     PA30R_PROCESS_TIME;
}PLC1_Thread8;

#define PLC1_THREAD8(T)((PLC1_Thread8*) T)


typedef struct
{
    uint8_t execute:1;
}Execute;


struct Result
{
    uint8_t DONE:1;
    uint8_t VALID:1;
    uint8_t ERROR:1;
    uint8_t PA60R_PROCESSED:1;
    uint8_t REWORK;
    DTL     PA30R_PROCESS_TIME;
}__attribute__((packed, aligned(1)));

typedef struct Result Result;


typedef struct
{
    PLC_String table;
    PLC_String frame_code;
}Frame;

static ThreadResult
step_wait(PLC1_Thread8 * self)
{
    Result result = 
    {
        .PA30R_PROCESS_TIME = DTL_DEFAULT
    };

    Execute execute;
    
    if(Cli_DBRead(self->super.client, PLC1_THREAD8_DB_INDEX, 14, sizeof(Execute), &execute) != 0 
        || Cli_DBWrite(self->super.client, PLC1_THREAD8_DB_INDEX, 0, sizeof(Result), &result) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == true)
    {
       Frame frame;

       if(Cli_DBRead(self->super.client, PLC1_THREAD8_DB_INDEX, 16, sizeof(Frame), &frame) != 0)
            return ThreadResult(.is_error = true); 

        frame.table.array[frame.table.length] = '\0';
        frame.frame_code.array[frame.frame_code.length] = '\0';

        TableContent * record = 
            model_get_table_content(
                self->super.model
                , frame.table.array
                , "PA30R_date_time, PA60R_date_time, PA60R_rework"
                , "frame_code"
                , frame.frame_code.array);

        if(record != NULL)
        {
            if(record->super.size == 1 
                && record->array[0]->super.size == 3
                && strcmp(record->array[0]->array[0], "*") != 0)
            {
                self->VALID = true;
                self->PA30R_PROCESS_TIME = DTL_DEFAULT;

                sscanf(
                    record->array[0]->array[0]
                    , "%hhd.%hhd.%hd*%hhd:%hhd"
                    , (signed char*) &self->PA30R_PROCESS_TIME.day
                    , (signed char*) &self->PA30R_PROCESS_TIME.month
                    , (short int*) &self->PA30R_PROCESS_TIME.year
                    , (signed char*) &self->PA30R_PROCESS_TIME.hour
                    , (signed char*)&self->PA30R_PROCESS_TIME.minute);

                self->PA30R_PROCESS_TIME.year = swap_endian(self->PA30R_PROCESS_TIME.year);
           
                if(strcmp(record->array[0]->array[1], "*") != 0)
                    self->PA60R_PROCESSED = true;
                else
                    self->PA60R_PROCESSED = false;

                self->REWORK = atoi(record->array[0]->array[2]);
            }
            else
            {
                self->VALID = false;
                self->REWORK = 0;
                self->PA60R_PROCESSED = false;
                self->PA30R_PROCESS_TIME = DTL_DEFAULT;
            }

            array_delete(ARRAY(record));
           
            return ThreadResult(.step = FINISH_TRUE);
        }
        else
            return ThreadResult(.step = FINISH_FALSE);
    
    }
    else
        return ThreadResult(.step = WAIT);
}


static ThreadResult
step_finish(PLC1_Thread8 * self)
{
    Execute execute;
    Result result = 
        {
            .DONE = true
            , .VALID = self->VALID
            , .PA60R_PROCESSED = self->PA60R_PROCESSED
            , .REWORK = self->REWORK
            , .PA30R_PROCESS_TIME = self->PA30R_PROCESS_TIME
        };
    
    if(Cli_DBRead(self->super.client, PLC1_THREAD8_DB_INDEX, 14, sizeof(Execute), &execute) != 0 
        || Cli_DBWrite(self->super.client, PLC1_THREAD8_DB_INDEX, 0, sizeof(Result), &result))
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);
    else
        return ThreadResult(.step = self->step);
}


static ThreadResult
step_error(PLC1_Thread8 * self)
{
    Result result = {.ERROR = true};
    Execute execute;
    
    if(Cli_DBRead(self->super.client, PLC1_THREAD8_DB_INDEX, 14, sizeof(Execute), &execute) != 0 
        || Cli_DBWrite(self->super.client, PLC1_THREAD8_DB_INDEX, 0, sizeof(Result), &result))
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

    switch(PLC1_THREAD8(self)->step)
    {
        case WAIT:
            if((result = step_wait(PLC1_THREAD8(self))).is_error == false)
                PLC1_THREAD8(self)->step = result.step;
            else
                return false;

            break;
        case FINISH_TRUE:
        case FINISH_FALSE:
            if((result = step_finish(PLC1_THREAD8(self))).is_error == false)
                PLC1_THREAD8(self)->step = result.step;
            else
                return false;

            break;
        case ERROR:
            if((result = step_error(PLC1_THREAD8(self))).is_error == false)
                PLC1_THREAD8(self)->step = result.step;
            else
                return false;
  
            break;
        default:
            PLC1_THREAD8(self)->step = WAIT;
    }
    
    return true;
}


PLC_Thread *
plc1_thread8_new(
    Model * model
    , S7Object client)
{
    PLC1_Thread8 * self = malloc(sizeof(PLC1_Thread8));

    if(self != NULL)
    {
        self->super = PLC_Thread(model, client, thread_run, (void(*)(PLC_Thread*)) free);
        self->step  = WAIT;
    }

    return PLC_THREAD(self);
}








