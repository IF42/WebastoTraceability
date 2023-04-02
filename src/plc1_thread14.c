#include "plc1_thread14.h"
#include "plc_thread_config.h"

#include <stdlib.h>
#include <string.h>
#include <endian.h>


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

    bool SHAKED;
    DTL LAST_SHAKE;
    PLC_String CHEMICAL_TYPE;
    uint8_t FILL_COUNT;
}PLC1_Thread14;

#define PLC1_THREAD14(T)((PLC1_Thread14*) T)


typedef struct
{
    uint8_t execute:1;
}Execute;    

typedef struct Result Result;

struct Result
{
    uint8_t DONE:1;
    uint8_t ERROR:1;
    uint8_t SHAKED:1;
    uint8_t unreachable;
    DTL LAST_SHAKE;
    PLC_String CHEMICAL_TYPE;
    uint8_t FILL_COUNT;
}__attribute__((packed, aligned(1)));


static ThreadResult
step_wait(PLC1_Thread14 * self)
{
    Execute execute;
    Result result = {.LAST_SHAKE = DTL_DEFAULT, .CHEMICAL_TYPE.size = 254};
    PLC_String batch_code;
    TableContent * record = NULL;

    if(Cli_DBRead(self->super.client, PLC1_THREAD14_DB_INDEX, 272, sizeof(Execute), &execute) != 0
       || Cli_DBWrite(self->super.client, PLC1_THREAD14_DB_INDEX, 0, sizeof(Result), &result) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);

    if(Cli_DBRead(self->super.client, PLC1_THREAD14_DB_INDEX, 274, sizeof(PLC_String), &batch_code) != 0)
        return ThreadResult(.is_error = true);

    batch_code.array[batch_code.length] = '\0';

    if((record = model_read_supplier_bottle(self->super.model, batch_code.array)) != NULL
        && record->super.size == 1)
    {
        strncpy(self->CHEMICAL_TYPE.array, record->array[0]->array[0], 253);
        self->CHEMICAL_TYPE.size = 254;
        self->CHEMICAL_TYPE.length = strlen(record->array[0]->array[0]);

        if(strcmp(record->array[0]->array[1], "*") != 0)
        {
            self->SHAKED = true;

            sscanf(
                record->array[0]->array[1]
                , "%hhd.%hhd.%hd*%hhd:%hhd"
                , (signed char*) &self->LAST_SHAKE.day
                , (signed char*) &self->LAST_SHAKE.month
                , (short int*) &self->LAST_SHAKE.year
                , (signed char*) &self->LAST_SHAKE.hour
                , (signed char*)&self->LAST_SHAKE.minute);

            self->LAST_SHAKE.year       = swap_endian(self->LAST_SHAKE.year);
            self->LAST_SHAKE.nanosecond = 0;
        }       
        else
        {
            self->LAST_SHAKE = DTL_DEFAULT;
            self->SHAKED = false;
        }

        self->FILL_COUNT = atoi(record->array[0]->array[2]);
        
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
step_finish(PLC1_Thread14 * self)
{
    Execute execute;
    Result result = 
        {
            .DONE            = true
            , .SHAKED        = self->SHAKED
            , .LAST_SHAKE    = self->LAST_SHAKE
            , .CHEMICAL_TYPE = self->CHEMICAL_TYPE
            , .FILL_COUNT    = self->FILL_COUNT
        };

    if(Cli_DBRead(self->super.client, PLC1_THREAD14_DB_INDEX, 272, sizeof(Execute), &execute) != 0
        || Cli_DBWrite(self->super.client, PLC1_THREAD14_DB_INDEX, 0, sizeof(Result), &result) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(execute.execute == false)
        return ThreadResult(.step = WAIT);
    else
        return ThreadResult(.step = FINISH);
}


static ThreadResult 
step_error(PLC1_Thread14 * self)
{
    Execute execute;
    Result result = 
        {
            .ERROR                = true
            , .LAST_SHAKE         = DTL_DEFAULT
            , .CHEMICAL_TYPE.size = 254
        };

    if(Cli_DBRead(self->super.client, PLC1_THREAD14_DB_INDEX, 272, sizeof(Execute), &execute) != 0
        || Cli_DBWrite(self->super.client, PLC1_THREAD14_DB_INDEX, 0, sizeof(Result), &result) != 0)
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

    switch(PLC1_THREAD14(self)->step)
    {
        case WAIT:
            if((result = step_wait(PLC1_THREAD14(self))).is_error == false)
                PLC1_THREAD14(self)->step = result.step;
            else
                return false;

            break;
        case FINISH:
            if((result = step_finish(PLC1_THREAD14(self))).is_error == false)
                PLC1_THREAD14(self)->step = result.step;
            else
                return false;

            break;
        case ERROR:
            if((result = step_error(PLC1_THREAD14(self))).is_error == false)
                PLC1_THREAD14(self)->step = result.step;
            else
                return false;

            break;
        default:
            PLC1_THREAD14(self)->step = WAIT;
    }

    return true;
}


PLC_Thread *
plc1_thread14_new(
    Model * model
    , S7Object client)
{
    PLC1_Thread14 * self = malloc(sizeof(PLC1_Thread14));

    if(self != NULL)
    {
        self->super = PLC_Thread(model, client, thread_run, (void(*)(PLC_Thread*)) free);
        self->step = WAIT;
    }

    return PLC_THREAD(self);
}
