#ifndef _PLC_READ_DB_H_
#define _PLC_READ_DB_H_

#include "plc_thread.h"

#include <snap7.h>
#include <stdint.h>


PLC_Thread *
plc_db_read_write_new(
	uint8_t db_index);

#endif
