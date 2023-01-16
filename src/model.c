#include "model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdlib.h>


//TODO: add mutex

Model *
model_init(void)
{
    Model * self = malloc(sizeof(Model));

    self->logout_timer = 0;

    FILE * log_stream = fopen(log_prepare_filename("log/log"), "w");

    if(log_stream != NULL)
        self->log = log_new(2, (FILE*[]) {stdout, log_stream});
    
    int rc = sqlite3_open("db/database.db", &self->db);
  
    if(rc != SQLITE_OK)
    {
        log_error(self->log, "SQLITE open error: %d", rc);
        model_delete(self);

        return NULL;
    }

    return self;
}


static void
array_string_delete(void* self)
{
    if(self != NULL)
    {
        for(size_t i = 0; i < ((ArrayString*)self)->super.size; i++)
        {
            if(((ArrayString*)self)->array[i] != NULL)
                free(((ArrayString*)self)->array[i]);
        }

        free(self);
    }
}



ArrayString *
model_get_table_list(Model * self)
{
    const char * sql = "SELECT name FROM sqlite_schema WHERE type ='table' AND name NOT LIKE 'sqlite_%';";
    sqlite3_stmt *res;

    int rc = sqlite3_prepare_v2(self->db, sql, -1, &res, 0);
    
    if (rc != SQLITE_OK) 
    {
        sqlite3_finalize(res);
        return NULL;
    }
     
    ArrayString * table_list = (ArrayString*) array_new(sizeof(char*), 0, array_string_delete);

    for(size_t i = 0; sqlite3_step(res) == SQLITE_ROW; i++)
    {
        table_list = (ArrayString*) array_resize(ARRAY(table_list), i+1);
        table_list->array[i] = strdup((char*) sqlite3_column_text(res, 0));
    }

    sqlite3_finalize(res);

    return table_list;
}


static void
table_content_delete(void * self)
{
    if(self != NULL)
    {
        for(size_t i = 0; i < ((TableContent*)self)->super.size; i++)
            array_delete(ARRAY(((TableContent*)self)->array[i]));

        free(self);
    }
}




TableContent *
model_get_table_content(
    Model * self
    , char * table
    , char * columns
    , char * key
    , char * value)
{
    char sql[512] = {0};
    
    if(key == NULL || value == NULL)
        sprintf(sql, "SELECT %s FROM '%s' LIMIT 4000;", columns, table);
    else
        sprintf(sql, "SELECT %s FROM %s WHERE %s LIKE '%s' LIMIT 4000;", columns, table, key, value);

    sqlite3_stmt *res;

    int rc = sqlite3_prepare_v2(self->db, sql, -1, &res, 0);
    
    if (rc != SQLITE_OK)
    {
        sqlite3_finalize(res);
        return NULL;
    }

    TableContent * db_table_content =  (TableContent*) array_new(sizeof(ArrayString*), 0, table_content_delete);

    for (size_t i = 0; sqlite3_step(res) == SQLITE_ROW; i++)
    {
        db_table_content = (TableContent*) array_resize(ARRAY(db_table_content), i+1);
        db_table_content->array[i] = (ArrayString*) array_new(sizeof(char*), 0, array_string_delete);
    
        char * string;
        for(size_t j = 0; (string = (char*) sqlite3_column_text(res, j)) != NULL; j++)
        {
            db_table_content->array[i] = (ArrayString*) array_resize(ARRAY(db_table_content->array[i]), j+1);
            db_table_content->array[i]->array[j] = strdup(string);
        }
    } 
    
    sqlite3_finalize(res);

    return db_table_content;
}


ArrayString *
model_get_table_columns(
    Model * self
    , char * table)
{
    char sql[512] = {0};  
    sprintf(sql, "PRAGMA table_info(%s)", table);

    sqlite3_stmt *res;

    int rc = sqlite3_prepare_v2(self->db, sql, -1, &res, 0);
    
    if (rc != SQLITE_OK)
    {
        sqlite3_finalize(res);
        return NULL;
    }
        
    ArrayString * columns = (ArrayString*) array_new(sizeof(char*), 0, array_string_delete);

    for(size_t i = 0; sqlite3_step(res) == SQLITE_ROW; i++)
    {
        columns = (ArrayString*) array_resize(ARRAY(columns), i+1);
        columns->array[i] = strdup((char*) sqlite3_column_text(res, 1));
    } 

    sqlite3_finalize(res);

    return columns;
}


bool
model_check_cover_exists(
    Model * self
    , char * cover_code)
{
    char query[512] = {0};
    sprintf(query, "SELECT cover_code FROM covers WHERE cover_code='%s';", cover_code);

    bool cover_exists = false;
    sqlite3_stmt *res;

    int rc = sqlite3_prepare_v2(self->db, query, -1, &res, NULL);
    
    if (rc == SQLITE_OK)
    {
        if(sqlite3_step(res) == SQLITE_ROW)
            cover_exists = true;
    }

    sqlite3_finalize(res);

    return cover_exists;
}


bool
model_write_new_cover(
    Model * self
    , char * cover_code
    , bool orientation
    , char * bottle1_batch
    , char * bottle2_batch
    , bool spare)
{
    char query[512] = {0};
    bool result = false;

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    sprintf(
        query
        , "INSERT INTO covers (cover_code, orientation, PA10B_date_time, PA10B_activator_left, PA10B_activator_right, spare) "
        "VALUES('%s', '%s', '%02d.%02d.%d*%02d:%02d', '%s', '%s', '%s');"
        , cover_code
        , orientation ? "left" : "right"
        , tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min
        , bottle1_batch
        , bottle2_batch
        , spare ? "YES" : "NO");

    int rc = sqlite3_exec(self->db, query, 0, 0, NULL);
    
    if (rc == SQLITE_OK)
        result = true;

    return result;
}


bool
model_write_new_frame(
    Model * self
    , char * table
    , char * frame_code
    , uint32_t order
    , char * primer_207_sika
    , char * remover_208_sika)
{
    char query[512] = {0};
    bool result = false;

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    sprintf(
        query
        , "INSERT INTO %s (frame_code, frame_order, PA30R_date_time, PA30R_Primer_207_SIKA, PA30R_Remover_208_SIKA, PA30R_rework)"
        " VALUES ('%s', '%d', '%02d.%02d.%d*%02d:%02d', '%s', '%s', '0')';" 
        , table
        , frame_code
        , order
        , tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min
        , primer_207_sika
        , remover_208_sika);

    int rc = sqlite3_exec(self->db, query, 0, 0, NULL);
    
    if (rc == SQLITE_OK)
        result = true;

    return result;
}


bool
model_write_environment_data(
    Model * self
    , float temperature
    , float humidity)
{
    bool result = false;

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    char query[512];
    sprintf(
        query
        , "INSERT INTO enviroment VALUES('%02d.%02d.%d*%02d:%02d:%02d', '%02.2f', '%02.2f');"
        , tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec
        , temperature
        , humidity);

    int rc = sqlite3_exec(self->db, query, 0, 0, NULL);
    
    if (rc == SQLITE_OK)
        result = true;

    return result;
}



//TODO: update also pa30r_produce_date_time
bool
model_update_frame_rewokrd(
    Model * self
    , char * table
    , char * frame_code
    , uint8_t rework)
{
    bool result = false;
    char query[512];

    sprintf(
        query
        , "UPDATE %s SET PA30R_rework='%d' WHERE frame_code='%s';"
        , table
        , rework
        , frame_code);
    
    int rc = sqlite3_exec(self->db, query, 0, 0, NULL);
    
    if (rc == SQLITE_OK)
        result = true;

    return result;   
}


void
model_delete(Model * self)
{
    if(self != NULL)
    {
        sqlite3_close(self->db);
        log_delete(self->log);
        free(self);
    }
}



