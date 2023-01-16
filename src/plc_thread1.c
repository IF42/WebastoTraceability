#include "plc_thread1.h"
#include <stdlib.h>


typedef struct PING
{
    uint8_t ping:1;
}PING;


typedef struct 
{
    PLC_Thread super;

    uint8_t db_index;
    uint8_t counter;
}PLC_Thread1;

#define PLC_Thread1(...)(PLC_Thread1){__VA_ARGS__}

#define PLC_THREAD1(T)((PLC_Thread1*) T)


static bool
plc_thread1_run(PLC_Thread * self)
{
    if(PLC_THREAD1(self)->counter >= 1)
    {
        PLC_THREAD1(self)->counter = 0;

        PING ping;

        if(Cli_DBRead(self->client, PLC_THREAD1(self)->db_index, 0, 1, &ping) != 0)
            return false;

        if(ping.ping == false)
        {
            ping.ping = true;

            if(Cli_DBWrite(self->client, PLC_THREAD1(self)->db_index, 0, 1, &ping) != 0)
                return false;
        }
    }
    else
        PLC_THREAD1(self)->counter++;    
    
    return true;
}


PLC_Thread *
plc_thread1_new(
    Model * model
    , S7Object client
    , uint8_t db_index)
{
    PLC_Thread1 * self = malloc(sizeof(PLC_Thread1));

    if(self != NULL)
    {
        self->super = PLC_Thread(model, client, plc_thread1_run,  (void(*)(PLC_Thread*)) free);
        self->db_index = db_index;
    }

    return PLC_THREAD(self);
}



