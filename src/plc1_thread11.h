#ifndef _PLC1_THREAD11_H_
#define _PLC1_THREAD11_H_

#include "plc_thread.h"


PLC_Thread *
plc1_thread11_new(
    Model * model
    , S7Object client
    , char * csv_path);

#endif
