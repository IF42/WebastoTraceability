#ifndef _PLC1_THREAD2_H_
#define _PLC1_THREAD2_H_ 

#include "plc_thread.h"

#include <snap7.h>
#include <stdint.h>

PLC_Thread *
plc_thread2_new(
    Model * model
    , S7Object client
    , uint8_t db_index);


#endif
