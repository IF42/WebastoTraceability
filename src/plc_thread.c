#include "plc_thread.h"


bool
plc_thread_run(PLC_Thread * self)
{
    return self->callback(self);
}


void
plc_thread_delete(PLC_Thread * self)
{
    self->delete(self);
}
