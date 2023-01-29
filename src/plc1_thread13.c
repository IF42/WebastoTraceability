#include "plc1_thread13.h"


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
}PLC1_Thread13;

#define PLC1_THREAD13(T)((PLC1_Thread13*) T)


static ThreadResult
step_wait(PLC1_Thread13 * self)
{

}


static ThreadResult
step_finish(PLC1_Thread13 * self)
{

}


static ThreadResult 
step_error(PLC1_Thread13 * self)
{

}


static bool
thread_run(PLC_Thread * self)
{
    ThreadResult result;

    switch(PLC1_THREAD13(self)->step)
    {
        case WAIT:
            if((result = step_wait(PLC1_THREAD13(self))).is_error == false)
                PLC1_THREAD13(self)->step = result.step;
            else
                return false;

            break;
        case FINISH:
            if((result = step_finish(PLC1_THREAD13(self))).is_error == false)
                PLC1_THREAD13(self)->step = result.step;
            else
                return false;

            break;
        case ERROR:
            if((result = step_error(PLC1_THREAD13(self))).is_error == false)
                PLC1_THREAD13(self)->step = result.step;
            else
                return false;

            break;
        default:
            PLC1_THREAD13(self)->step = WAIT;
    }

    return true;
}


PLC_Thread *
plc1_thread13_new(
    Model * model
    , S7Object client)
{
    PLC1_Thread13 * self = malloc(sizeof(PLC1_Thread13));

    if(self != NULL)
    {
        self->super = PLC_Thread(model, client, thread_run, (void(*)(PLC_Thread*)) free);
        self->step = WAIT;
    }

    return PLC_THREAD(self);
}
