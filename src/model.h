#ifndef _MODEL_H_
#define _MODEL_H_ 

#include <time.h>
#include <sqlite3.h>
#include <log.h>
#include <array.h>
#include <stdint.h>

typedef struct
{
    Array super;
    char * array[];
}ArrayString;


typedef struct
{
    Array super;
    ArrayString * array[];
}TableContent;


typedef struct
{
    time_t logout_timer;
    sqlite3 * db;
    Log * log;
    
    char logged_user[64];
}Model;

#define Model(...)(Model){__VA_LIST__}



Model *
model_init(void);


ArrayString *
model_get_table_list(Model * self);


ArrayString *
model_get_table_columns(
    Model * self
    , char * table);


TableContent *
model_get_table_content(
    Model * self
    , char * table
    , char * columns
    , char * key
    , char * value);


bool
model_check_cover_exists(
    Model * self
    , char * cover_code);


bool
model_write_new_cover(
    Model * self
    , char * cover_code
    , bool orientation
    , char * bottle1_batch
    , char * bottle2_batch
    , bool spare);


bool
model_write_new_frame(
    Model * self
    , char * table
    , char * frame_code
    , uint32_t order
    , char * primer_207_sika
    , char * remover_208_sika);


bool
model_write_environment_data(
    Model * self
    , float temperature
    , float humidity);


bool
model_update_frame_rewokrd(
    Model * self
    , char * table
    , char * frame_code
    , uint8_t rework);


void
model_delete(Model * self);

#endif
