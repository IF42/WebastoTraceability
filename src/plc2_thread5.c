#include "plc2_thread5.h"

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
}PLC2_Thread5;

#define PLC2_THREAD5(T)((PLC2_Thread5*) T)


static ThreadResult
step_wait(PLC2_Thread5 * self)
{

}


static ThreadResult
step_finish(PLC2_Thread5 * self)
{

}


static ThreadResult 
step_error(PLC2_Thread5 * self)
{

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
    }

    return PLC_THREAD(self);
}
