#ifndef _MODEL_H_
#define _MODEL_H_ 

#include <time.h>
#include <sqlite3.h>
#include <log.h>
#include <array.h>
#include <stdint.h>
#include <pthread.h>


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

    pthread_mutex_t mutex;

}Model;

#define Model(...)(Model){__VA_LIST__}



Model *
model_init(void);


TableContent *
model_get_table_list(Model * self);


TableContent *
model_get_table_columns(
    Model * self
    , char * table);


TableContent *
model_get_table_content(
    Model * self
    , size_t limit
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
model_pa30r_update_frame_rework(
    Model * self
    , char * table
    , char * frame_code
    , uint8_t rework);


bool
model_pa60r_update_frame_rework(
    Model * self
    , char * table
    , char * frame_code
    , uint8_t rework);


bool
model_pa60r_update_frame(
     Model * self
    , char * table
    , char * frame_code);


bool
model_pa30b_update_frame(
     Model * self
    , char * frame_code
    , char * left_cover_code
    , char * right_cover_code);


bool
model_generate_frame_csv(
    Model * self
    , char * csv_path
    , char * table
    , char * frame_code
    , int8_t temperature
    , int8_t humidity);



bool
model_supplier_bottle_exists(
    Model * self
    , char * bottle_batch_code);


bool
model_write_supplier_bottle(
    Model * self
    , char * chemical_type
    , char * batch_code);


TableContent *
model_read_supplier_bottle(
    Model * self
    , char * batch_code);


bool
model_write_glue_barrel(
    Model * self
    , char * material_description
    , char * material_type
    , char * batch_code
    , char * validity);


TableContent *
model_read_cover(
    Model * self
    , char * cover_code);


bool
model_set_cover_used(
    Model * self
    , char * cover_code);


TableContent *
model_read_frame(
    Model * self
    , char * frame_code);


bool
model_update_bottle_fill_counter(
    Model * self
    , char * batch_code
    , uint8_t fill_counter);


bool
model_update_bottle_shake_time(
    Model * self
    , char * batch_code);


bool
model_delete_record(
    Model * self
    , char * table
    , size_t length
    , const char ** columns
    , const char ** values);


void
model_delete(Model * self);


#endif
