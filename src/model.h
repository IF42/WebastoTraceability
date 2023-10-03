#ifndef _MODEL_H_
#define _MODEL_H_ 

#include <time.h>
#include <log.h>
#include <sqlite3.h>
#include <stdint.h>
#include <pthread.h>


typedef struct
{
    time_t logout_timer;
    sqlite3 * db;
    Log * log;

    char logged_user[64];

    pthread_mutex_t mutex;
}Model;


#define Model(...)(Model){__VA_LIST__}


Model *
model_init(void);


bool
model_db_write(
    Model * self
    , const char * query_format
    , ...);


bool
model_delete_record(
    Model * self
    , char * table
    , size_t length
    , const char ** columns
    , const char ** values);


void
model_delete(Model * self);


bool
model_write_environment_data(
    Model * self
    , float temperature
    , float humidity);


bool
model_db_callback_read(
	Model * self
	, int (*callback)(void *, int, char **, char **)
	, void * param 
    , const char * format
	, ...);

bool
model_export_mes_csv(
    Model * self
    , char * csv_path
    , char * table
    , char * frame_code
    , int8_t temperature
    , int8_t humidity);

#endif
