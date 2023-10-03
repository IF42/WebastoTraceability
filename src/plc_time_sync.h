#ifndef _PLC1_THREAD2_H_
#define _PLC1_THREAD2_H_ 

#include "plc_thread.h"

#include <snap7.h>
#include <stdint.h>

PLC_Thread *
plc_time_sync_new(
		uint8_t db_index);


#endif
