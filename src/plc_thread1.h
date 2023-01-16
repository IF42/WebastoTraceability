#ifndef _PLC_THREAD1_H_
#define _PLC_THREAD1_H_

#include "plc_thread.h"

#include <snap7.h>
#include <stdint.h>


#define PLC1_PING_DB_INDEX 19
#define PLC2_PING_DB_INDEX 8



PLC_Thread *
plc_thread1_new(
    Model * model
    , S7Object client
    , uint8_t db_index);

#endif
