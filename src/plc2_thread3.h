#ifndef _PLC2_THREAD3_H_
#define _PLC2_THREAD3_H_

#include "plc_thread.h"

#include <snap7.h>
#include <stdint.h>


PLC_Thread *
plc2_thread3_new(
    Model * model
    , S7Object client);


#endif
