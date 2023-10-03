#ifndef _PLC_THREAD_H_
#define _PLC_THREAD_H_ 

#include "model.h"

#include <snap7.h>
#include <stdbool.h>


typedef struct PLC_Thread
{
    bool (*callback)(struct PLC_Thread *, Model *, S7Object);
    void (*delete)(struct PLC_Thread *);
}PLC_Thread;


#define PLC_Thread(...)(PLC_Thread){__VA_ARGS__}
#define PLC_THREAD(T)((PLC_Thread*) T)


bool
plc_thread_run(
		PLC_Thread * self
		, Model * model
		, S7Object client);


char *
cli_error(int cli_result);


void
plc_thread_delete(PLC_Thread * self);


#endif






