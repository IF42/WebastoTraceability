#include "plc2_thread3.h"
#include "plc_thread_config.h"

#include <time.h>
#include <stdlib.h>
#include <endian.h>


/* reading interval in seconds */
#define READING_INTERVAL 60


struct Environment
{   
    int16_t temperature;
    int16_t humidity;
}__attribute__((packed, aligned(1)));


typedef struct Environment Environment;


typedef enum
{
    WAIT
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
    time_t last_write;
}PLC2_Thread3;


#define PLC2_Thread3(...)(PLC2_Thread3){__VA_ARGS__}


#define PLC2_THREAD3(T)((PLC2_Thread3*) T)


static ThreadResult
step_wait(PLC2_Thread3 * self)
{
    Environment environment;
    time_t cur_t  = time(NULL);
    uint8_t error = 0;

    
    if(Cli_DBWrite(self->super.client, PLC2_THREAD3_DB_INDEX, 6, 1, &error) != 0)
        return ThreadResult(.is_error = true);

    if(difftime(cur_t, self->last_write) < READING_INTERVAL)
        return ThreadResult(.step = WAIT);

    if(Cli_DBRead(self->super.client, PLC2_THREAD3_DB_INDEX, 2, sizeof(Environment), &environment) != 0)
        return ThreadResult(.is_error = true);
   
    if(model_write_environment_data(
        self->super.model
        , swap_endian(environment.temperature) / 100.0
        , swap_endian(environment.humidity) / 100.0) == true)
    {
        self->last_write = cur_t;
        return ThreadResult(.step = WAIT);
    }
    else
        return ThreadResult(.step = ERROR);
}


static ThreadResult
step_error(PLC2_Thread3 * self)
{
    uint8_t error = 0x01;
    uint8_t reset;

    if(Cli_DBWrite(self->super.client, PLC2_THREAD3_DB_INDEX, 6, 1, &error) != 0
        || Cli_DBRead(self->super.client, PLC2_THREAD3_DB_INDEX, 0, 1, &reset) != 0)
    {
        return ThreadResult(.is_error = true);
    }

    if(reset == true)
        return ThreadResult(.step = WAIT);
    else
        return ThreadResult(.step = ERROR);
}


bool
plc2_thread3_run(PLC_Thread * self)
{
    ThreadResult result;

    switch(PLC2_THREAD3(self)->step)
    {
        case WAIT:
            if((result = step_wait(PLC2_THREAD3(self))).is_error == false)
                PLC2_THREAD3(self)->step = result.step;
            else
                return false;

            break;
        case ERROR:
            if((result = step_error(PLC2_THREAD3(self))).is_error == false)
                PLC2_THREAD3(self)->step = result.step;
            else
                return false;

            break;
        default:
            PLC2_THREAD3(self)->step = WAIT;
    }

    return true;
}


PLC_Thread *
plc2_thread3_new(
    Model * model
    , S7Object client)
{
    PLC2_Thread3 * self = malloc(sizeof(PLC2_Thread3));

    if(self != NULL)
    {
        self->super      = PLC_Thread(model, client, plc2_thread3_run, (void(*)(PLC_Thread*)) free);
        self->step       = WAIT;
        self->last_write = 0;
    }

    return PLC_THREAD(self);
}


