#include "plc2_thread4.h"
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
    uint8_t execute:1;
}Execute;

typedef struct Result Result;

struct Result
{
    uint8_t DONE:1;
    uint8_t ERROR:1;
    uint8_t PA60R_PROCESSED:1;
    uint8_t unreachable;
    DTL PA30R_PROCESS_DATE_TIME;
    PLC_String order;
}__attribute__((packed, aligned(1)));




typedef struct 
{
    PLC_Thread super;

    ThreadStep step;
    
    bool pa60r_processed;
    DTL pa30r_process_date_time;
    PLC_String order;
}PLC2_Thread4;

#define PLC2_Thread4(...)(PLC2_Thread4) {__VA_ARGS__}

#define PLC2_THREAD4(T)((PLC2_Thread4*) T)


static ThreadResult
step_wait(PLC2_Thread4 * self)
{
    Result result = {.PA30R_PROCESS_DATE_TIME = DTL_DEFAULT, .order.size = 254};
    PLC_String frame_code;
    Execute execute;
    TableContent * record;

    if(Cli_DBWrite(self->super.client, PLC2_THREAD4_DB_INDEX, 0, sizeof(Result), &result) != 0
        || Cli_DBRead(self->super.client, PLC2_THREAD4_DB_INDEX, 270, sizeof(Execute), &execute) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);

    if(Cli_DBRead(self->super.client, PLC2_THREAD4_DB_INDEX, 272, sizeof(PLC_String), &frame_code) != 0)
        return ThreadResult(.is_error = true);

    frame_code.array[frame_code.length] = '\0';

    if((record = model_read_frame(
            self->super.model
            , frame_code.array)) != NULL
        && record->super.size == 1)
    {
        sscanf(
            record->array[0]->array[0]
            , "%hhd.%hhd.%hd*%hhd:%hhd"
            , (int8_t*) &self->pa30r_process_date_time.day
            , (int8_t*) &self->pa30r_process_date_time.month
            , (short int *) &self->pa30r_process_date_time.year
            , (int8_t*) &self->pa30r_process_date_time.hour
            , (int8_t*) &self->pa30r_process_date_time.minute);

        printf("%d.%d.%d %d:%d\n "
            , self->pa30r_process_date_time.day
            , self->pa30r_process_date_time.month
            , self->pa30r_process_date_time.year
            , self->pa30r_process_date_time.hour
            , self->pa30r_process_date_time.minute);
        self->pa30r_process_date_time.year = swap_endian(self->pa30r_process_date_time.year);
        self->pa30r_process_date_time.nanosecond = 0;
        self->pa30r_process_date_time.second = 0;

        if(strcmp(record->array[0]->array[1], "*") != 0)
            self->pa60r_processed = true;
        else
            self->pa60r_processed = false;

        size_t length = strlen(record->array[0]->array[2]);

        if(length >= 254)
            length = 254;

        memcpy(self->order.array, record->array[0]->array[2], length);
        self->order.length = length;
        self->order.size = 254;

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
step_finish(PLC2_Thread4 * self)
{
    Result result = 
        {
            .DONE                      = true
            , .ERROR                   = false
            , .PA60R_PROCESSED         = self->pa60r_processed
            , .PA30R_PROCESS_DATE_TIME = self->pa30r_process_date_time
            , .order                   = self->order
        };
    Execute execute; 

    if(Cli_DBWrite(self->super.client, PLC2_THREAD4_DB_INDEX, 0, sizeof(Result), &result) != 0
        || Cli_DBRead(self->super.client, PLC2_THREAD4_DB_INDEX, 270, sizeof(execute), &execute) != 0)
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
    Execute execute;
    Result result = 
        {
            .ERROR = true
            , .PA30R_PROCESS_DATE_TIME = DTL_DEFAULT
            , .order.size = 254
        };
    
    if(Cli_DBWrite(self->super.client, PLC2_THREAD4_DB_INDEX, 0, sizeof(Result), &result) != 0
        || Cli_DBRead(self->super.client, PLC2_THREAD4_DB_INDEX, 270, sizeof(execute), &execute) != 0)
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


