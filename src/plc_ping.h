#ifndef _PLC_THREAD1_H_
#define _PLC_THREAD1_H_

#include "plc_thread.h"

#include <snap7.h>
#include <stdint.h>


PLC_Thread *
plc_ping_new(
	uint8_t db_index);

#endif
