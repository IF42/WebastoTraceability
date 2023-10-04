#include "controller.h"
#include "plc_ping.h"
#include "plc_time_sync.h"
#include "plc_sql_read_write.h"
#include "plc_env_log.h"
#include "plc_mes_csv.h"

#include <sys/time.h>
#include <stdlib.h>
#include <snap7.h>


// ms
#define CYCLE_TIME 500


#define PLC1_IP_ADDRESS "192.168.0.1"
#define PLC1_RACK 0
#define PLC1_SLOT 1   


#define PLC2_IP_ADDRESS "192.168.0.20"
#define PLC2_RACK 0
#define PLC2_SLOT 1   


#define PLC1_PING_DB_INDEX 1
#define PLC1_TIME_SYNC_DB_INDEX 2
#define PLC1_SQL_READ_WRITE_DB_INDEX 3
#define PLC1_WRITE_CSV_DB_INDEX 4

#define PLC2_PING_DB_INDEX 41
#define PLC2_TIME_SYNC_DB_INDEX 2
#define PLC2_SQL_READ_WRITE_DB_INDEX 44
#define PLC2_WRITE_CSV_DB_INDEX 4
#define PLC2_ENV_LOG_DB_INDEX 45


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


#define PLC1_THREAD_POOL_SIZE 4
#define PLC2_THREAD_POOL_SIZE 5


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
	switch(process->status)
	{
	case CONNECTED:
		for(uint8_t i = 0; i < process->thread_pool_size; i++)
		{
			if(plc_thread_run(process->thread_pool[i], model, process->client) == false)
			{
				process->status = RECONNECT;
				log_error(model->log, "%s:%d: %s thread id: %d connection error", __FILE__, __LINE__, process_name, i+1);
				break;
			}
		 }
		 break;
	case DISCONNECTED:
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
			 {
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
		 }
		 else
			 process->connection_delay ++;

		 break;
	default:
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


static Controller instance;
static Controller * ptr;


Controller *
controller_new(Model * model)
{
    if(ptr == NULL)
    {
        instance.model = model;
        
        instance.plc1 = (PLC_Process) 
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

        instance.plc2 = (PLC_Process) 
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

        instance.plc1.thread_pool[0] = plc_ping_new(PLC1_PING_DB_INDEX);
        instance.plc1.thread_pool[1] = plc_time_sync_new(PLC1_TIME_SYNC_DB_INDEX);
        instance.plc1.thread_pool[2] = plc_db_read_write_new(PLC1_SQL_READ_WRITE_DB_INDEX);
        instance.plc1.thread_pool[3] = plc_mes_csv_new(PLC1_WRITE_CSV_DB_INDEX);

        instance.plc2.thread_pool[0] = plc_ping_new(PLC2_PING_DB_INDEX);
        instance.plc2.thread_pool[1] = plc_time_sync_new(PLC2_TIME_SYNC_DB_INDEX);
        instance.plc2.thread_pool[2] = plc_db_read_write_new(PLC2_SQL_READ_WRITE_DB_INDEX);
        instance.plc1.thread_pool[3] = plc_mes_csv_new(PLC2_WRITE_CSV_DB_INDEX);
        instance.plc2.thread_pool[4] = plc_env_log_new(PLC2_ENV_LOG_DB_INDEX);

        pthread_create(
            &instance.thread_id
            , NULL
            , (void*(*)(void*)) controller_thread
            , &instance);

        ptr = &instance;
    }
    
    return ptr;
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
    }
}



