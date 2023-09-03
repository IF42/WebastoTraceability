#include "controller.h"
#include "plc_thread_config.h"

#include "plc_thread1.h"
#include "plc_thread2.h"
#include "plc1_thread3.h"
#include "plc1_thread4.h"
#include "plc1_thread5.h"
#include "plc1_thread6.h"
#include "plc1_thread7.h"
#include "plc1_thread8.h"
#include "plc1_thread9.h"
#include "plc1_thread10.h"
#include "plc1_thread11.h"
#include "plc1_thread12.h"
#include "plc1_thread13.h"
#include "plc1_thread14.h"
#include "plc1_thread15.h"
#include "plc1_thread16.h"

#include "plc2_thread3.h"
#include "plc2_thread4.h"
#include "plc2_thread5.h"
#include "plc2_thread6.h"
#include "plc2_thread7.h"
#include "plc2_thread8.h"
#include "plc2_thread9.h"

#include <sys/time.h>
#include <stdlib.h>
#include <snap7.h>


#define CYCLE_TIME 1000  // ms



#define PLC1_IP_ADDRESS "192.168.0.1"
#define PLC1_RACK 0
#define PLC1_SLOT 1   


#define PLC2_IP_ADDRESS "192.168.0.20"
#define PLC2_RACK 0
#define PLC2_SLOT 1   


typedef enum
{
    DISCONNECTED
    , CONNECTED
    , RECONNECT
}ConnectionStatus;


/**
** if the cycle interval is 500ms, this means reconnection after 5 seconds
*/
#define RECONNECT_DELAY 10


#define PLC1_THREAD_POOL_SIZE 16
#define PLC2_THREAD_POOL_SIZE 9


typedef struct
{
    S7Object client;
    ConnectionStatus status;
    uint8_t connection_delay;
    
    char * ip_address;
    int rack;
    int slot;

    uint8_t thread_pool_size;
    PLC_Thread ** thread_pool;
}PLC_Process;


struct Controller
{
    Model * model;

    PLC_Process plc1;
    PLC_Process plc2;

    pthread_t thread_id;
};


#define Controller(...) (Controller){__VA_ARGS__}


static void
controller_run_process(
    PLC_Process * process
    , const char * process_name
    , Model * model)
{
    if(process->status == CONNECTED)
    {
        for(uint8_t i = 0; i < process->thread_pool_size; i++)
        {
            if(plc_thread_run(process->thread_pool[i]) == false)
            {
                process->status = RECONNECT;
                log_error(model->log, "%s:%d: %s thread id: %d connection error", __FILE__, __LINE__, process_name, i+1);
                break;
            }
        }
    }
    else if(process->status == DISCONNECTED)
    {
        // because this is called in main thread, it is protection against freezing if the plc not responding
        if(process->connection_delay >= RECONNECT_DELAY)
        {
            process->connection_delay = 0;

            if(Cli_ConnectTo(
                process->client
                , process->ip_address
                , process->rack
                , process->slot) == 0)
            {
                process->status = CONNECTED;
                log_debug(
                    model->log
                    , "%s connected to ip: %s rack: %d slot: %d"
                    , process_name
                    , process->ip_address
                    , process->rack
                    , process->slot);
            }
            else
                log_error(
                    model->log
                    , "%s:%d: %s connection error to ip: %s rack: %d slot: %d"
                    , __FILE__
                    , __LINE__
                    , process_name
                    , process->ip_address
                    , process->rack
                    , process->slot);
        }
        else
            process->connection_delay ++;
    }
    else
    {
        Cli_Disconnect(process->client);
        log_debug(model->log, "%s network reconnecting", process_name);
        process->status = DISCONNECTED;
    }    
}


static void *
controller_thread(Controller * self)
{
    struct timeval start, end;
    uint32_t elapsed_time;
    long seconds, useconds;

    while(true)
    {
        gettimeofday(&start, NULL);

        controller_run_process(&self->plc1, "plc1", self->model);
        controller_run_process(&self->plc2, "plc2", self->model);

        gettimeofday(&end, NULL);
        seconds = end.tv_sec - start.tv_sec;
        useconds = end.tv_usec - start.tv_usec;

        elapsed_time = (uint32_t) (((seconds) * 1000 + useconds / 1000.0) + 0.5);

        if(elapsed_time < CYCLE_TIME)
        {
            uint32_t cycle_delay = CYCLE_TIME - elapsed_time;
            struct timespec delay = 
                {
                    .tv_sec = cycle_delay / 1000
                    , .tv_nsec = (cycle_delay % 1000) * 1000000
                };

            nanosleep(&delay, NULL);
        }
    }
    
    return NULL;
}


Controller *
controller_init(Model * model)
{
    Controller * self = malloc(sizeof(Controller));

    if(self != NULL)
    {
        self->model = model;
        
        self->plc1 = (PLC_Process) 
            {
                .client             = Cli_Create()
                , .status           = DISCONNECTED
                , .connection_delay = RECONNECT_DELAY
                , .ip_address       = PLC1_IP_ADDRESS
                , .rack             = PLC1_RACK
                , .slot             = PLC1_SLOT 
                , .thread_pool_size = PLC1_THREAD_POOL_SIZE
                , .thread_pool      = malloc(sizeof(PLC_Thread *) * PLC1_THREAD_POOL_SIZE)
            };

        self->plc2 = (PLC_Process) 
            {
                .client             = Cli_Create()
                , .status           = DISCONNECTED
                , .connection_delay = RECONNECT_DELAY
                , .ip_address       = PLC2_IP_ADDRESS
                , .rack             = PLC2_RACK
                , .slot             = PLC2_SLOT 
                , .thread_pool_size = PLC2_THREAD_POOL_SIZE
                , .thread_pool      = malloc(sizeof(PLC_Thread *) * PLC2_THREAD_POOL_SIZE)
            };

        self->plc1.thread_pool[0]  = plc_thread1_new(model, self->plc1.client, PLC1_THREAD1_DB_INDEX);
        self->plc1.thread_pool[1]  = plc_thread2_new(model, self->plc1.client, PLC1_THREAD2_DB_INDEX);
        self->plc1.thread_pool[2]  = plc1_thread3_new(model, self->plc1.client);
        self->plc1.thread_pool[3]  = plc1_thread4_new(model, self->plc1.client);
        self->plc1.thread_pool[4]  = plc1_thread5_new(model, self->plc1.client);
        self->plc1.thread_pool[5]  = plc1_thread6_new(model, self->plc1.client);
        self->plc1.thread_pool[6]  = plc1_thread7_new(model, self->plc1.client);
        self->plc1.thread_pool[7]  = plc1_thread8_new(model, self->plc1.client);
        self->plc1.thread_pool[8]  = plc1_thread9_new(model, self->plc1.client);
        self->plc1.thread_pool[9]  = plc1_thread10_new(model, self->plc1.client);
        self->plc1.thread_pool[10] = plc1_thread11_new(model, self->plc1.client);
        self->plc1.thread_pool[11] = plc1_thread12_new(model, self->plc1.client);
        self->plc1.thread_pool[12] = plc1_thread13_new(model, self->plc1.client);
        self->plc1.thread_pool[13] = plc1_thread14_new(model, self->plc1.client);
        self->plc1.thread_pool[14] = plc1_thread15_new(model, self->plc1.client);
        self->plc1.thread_pool[15] = plc1_thread16_new(model, self->plc1.client);

        self->plc2.thread_pool[0] = plc_thread1_new(model, self->plc2.client, PLC2_THREAD1_DB_INDEX);
        self->plc2.thread_pool[1] = plc_thread2_new(model, self->plc2.client, PLC2_THREAD2_DB_INDEX);
        self->plc2.thread_pool[2] = plc2_thread3_new(model, self->plc2.client);
        self->plc2.thread_pool[3] = plc2_thread4_new(model, self->plc2.client);
        self->plc2.thread_pool[4] = plc2_thread5_new(model, self->plc2.client);
        self->plc2.thread_pool[5] = plc2_thread6_new(model, self->plc2.client);
        self->plc2.thread_pool[6] = plc2_thread7_new(model, self->plc2.client);
        self->plc2.thread_pool[7] = plc2_thread8_new(model, self->plc2.client);
        self->plc2.thread_pool[8] = plc2_thread9_new(model, self->plc2.client);

        pthread_create(
            &self->thread_id
            , NULL
            , (void*(*)(void*)) controller_thread
            , self); 
    }
    
    return self;
}


void
controller_delete(Controller * self)
{
    if(self != NULL)
    {
        for(uint8_t i = 0; i < self->plc1.thread_pool_size; i++)
            plc_thread_delete(self->plc1.thread_pool[i]);

        for(uint8_t i = 0; i < self->plc2.thread_pool_size; i++)
            plc_thread_delete(self->plc2.thread_pool[i]);

        free(self);
    }
}



