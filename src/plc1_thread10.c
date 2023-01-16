#include "plc1_thread10.h"

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

#define ThreadResult(...)(ThreadState){__VA_ARGS__}


typedef struct
{
    PLC_Thread super;
    ThreadState step;
}PLC1_Thread10;

#define PLC1_THREAD10(T)((PLC1_Thread10*) T)


static ThreadResult 
step_wait(PLC1_Thread10 * self)
{
    
}


static ThreadResult
step_finish(PLC1_Thread10 * self)
{

}


static ThreadResult
step_error(PLC1_Thread10 * self)
{

}


static bool
thread_run(PLC_Thread * self)
{
    ThreadResult result;

    switch(PLC1_THREAD10(self)->step)
    {
        case WAIT:
            if((result = step_wait(PLC1_THREAD10(self))).is_error == false)
                PLC1_THREAD10(self)->step = result.step;
            else
                return false;

            break;
        case FINISH:
            if((result = step_finish(PLC1_THREAD10(self))).is_error == false)
                PLC1_THREAD10(self)->step = result.step;
            else
                return false;

            break;
        case ERROR:
            if((result = step_error(PLC1_THREAD10(self))).is_error == false)
                PLC1_THREAD10(self)->step = result.step;
            else
                return false;

            break;
        default:
            PLC1_THREAD10(self)->step = WAIT;
    }

    return false;
}


PLC_Thread *
plc1_thread10_new(
    Model * model
    , S7Object client)
{
    PLC1_Thread10 * self = malloc(sizeof(PLC1_Thread10));

    if(self != NULL)
    {
        self->super = PLC_Thread(model, client, thread_run, (void(*)(PLC_Thread*)) free);
        self->step = WAIT;   
    }

    return PLC_THREAD(self);
}




