#include "model.h"
#include "version.h"

#include <util.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>


static Model instance;
static Model * ptr;

Model *
model_init(void)
{
    if(ptr == NULL)
    {
        instance.logout_timer = 0;

        FILE * log_stream = fopen(log_prepare_filename("log/log"), "w");

        if(log_stream != NULL)
        {
            instance.log = 
                log_new(
                    2
                    , (FILE*[]) {stdout, log_stream}
                    , "Log init (%s: %s)"
                    ,  __progname__
                    , __version__);
        }
        else
        {
            fprintf(stderr, "ERROR open log file!\n");
            model_delete(&instance);
            
            return NULL;
        }
        
        int rc = sqlite3_open("db/database.db", &instance.db);
      
        if(rc != SQLITE_OK)
        {
            log_error(instance.log, "SQLITE open error: (%d) %s", rc, sqlite3_errmsg(instance.db));
            model_delete(&instance);

            return NULL;
        }

        pthread_mutex_init(&instance.mutex, NULL);
        ptr = &instance;
    }

    return ptr;
}


static void
table_content_delete(TableContent * self)
{
    if(self == NULL)
        return;
    
    for(size_t i = 0; i < self->super.size; i++)
        array_delete(ARRAY(self->array[i]));

    free(self);
}


static void
array_string_delete(ArrayString * self)
{
    if(self == NULL) 
        return;
    
    for(size_t i = 0; i < self->super.size; i ++)
    {
        if(self->array[i] != NULL)
            free(self->array[i]);
    }

    free(self);
}


TableContent *
model_db_read(
    Model * self
    , const char * query_format
    , ...)
{
	va_list args;
	char sql_query[512];
	sqlite3_stmt * res;

	va_start(args, query_format);
	vsnprintf(sql_query, 511, query_format, args);
	va_end(args);

	pthread_mutex_lock(&self->mutex);

	int rc = sqlite3_prepare_v2(self->db, sql_query, -1, &res, 0);

	if (rc != SQLITE_OK)
	{
		log_error(self->log, "SQLITE read table error: (%d) %s", rc, sqlite3_errmsg(self->db));
		pthread_mutex_unlock(&self->mutex);

		return NULL;
	}

	TableContent * db_table_content =
		(TableContent*) array_new(
							sizeof(ArrayString*)
							, 0
							, (void (*)(void*))table_content_delete);

	for (size_t i = 0; sqlite3_step(res) == SQLITE_ROW; i++)
	{
		db_table_content =
			(TableContent*) array_resize(
								ARRAY(db_table_content)
								, i+1);
		db_table_content->array[i] =
			(ArrayString*) array_new(
								sizeof(char*)
								, 0
								, (void(*)(void*)) array_string_delete);

		char * string;

		for(size_t j = 0; (string = (char*) sqlite3_column_text(res, j)) != NULL; j++)
		{
			db_table_content->array[i] =
				(ArrayString*) array_resize(
									ARRAY(db_table_content->array[i])
									, j+1);

			db_table_content->array[i]->array[j] = strdup(string);
		}
	}

	sqlite3_finalize(res);
	pthread_mutex_unlock(&self->mutex);

	return db_table_content;
}

static bool
model_db_write(
    Model * self
    , const char * query_format
    , ...)
{
    va_list args;
    char sql_query[1024];
    bool result = false;

    va_start(args, query_format);
    vsnprintf(sql_query, 1023, query_format, args);
    va_end(args);
 
    pthread_mutex_lock(&self->mutex);

    int rc = sqlite3_exec(self->db, sql_query, 0, 0,  NULL);

    if (rc == SQLITE_OK)
        result = true;
    else
        log_error(self->log, "SQLITE write error: (%d) %s", rc, sqlite3_errmsg(self->db));

    pthread_mutex_unlock(&self->mutex);

    return result;
}


TableContent *
model_get_table_list(Model * self )
{
    return model_db_read(
                self
                , "SELECT name FROM sqlite_schema WHERE type='table' AND name NOT LIKE \"sqlite_%\";");
}


TableContent * 
model_get_table_content(
    Model * self
    , size_t limit
    , char * table
    , char * columns
    , char * key
    , char * value)
{
    if(key == NULL 
        || value == NULL 
        || (value != NULL && strcmp(value, "") == 0) 
        || (key != NULL && strcmp(key, "") == 0))
    {
        if(limit == 0)
        {
            return model_db_read(
                        self
                        , "SELECT %s FROM '%s' ORDER BY rowid DESC;"
                        , columns
                        , table);
        }
        else
        {
            return model_db_read(
                        self
                        , "SELECT %s FROM '%s' ORDER BY rowid DESC LIMIT %ld;"
                        , columns
                        , table
                        , limit);
        }
    }
    else
    {
        if(limit == 0)
        {
            return model_db_read(
                        self
                        , "SELECT %s FROM %s WHERE %s LIKE '%s' ORDER BY rowid DESC;"
                        , columns
                        , table
                        , key
                        , value);
        }
        else
        {
            return model_db_read(
                        self
                        , "SELECT %s FROM %s WHERE %s LIKE '%s' ORDER BY rowid DESC LIMIT %ld;"
                        , columns
                        , table
                        , key
                        , value
                        , limit);
        }
    }
}


bool
model_delete_record(
    Model * self
    , char * table
    , size_t length
    , const char ** columns
    , const char ** values)
{
    char sql_query[2048];
    bool result = true;
    int rc;
    
    sprintf(sql_query, "DELETE FROM %s WHERE ", table);
    
    for(size_t i = 0; i < length; i++)
    {
        if(i > 0)
            strcat(sql_query, " AND ");

        strcat(sql_query, columns[i]);
        strcat(sql_query, "='");
        strcat(sql_query, values[i]);
        strcat(sql_query, "'");
    }

    strcat(sql_query, ";");

    pthread_mutex_lock(&self->mutex);

    if ((rc  = sqlite3_exec(self->db, sql_query, 0, 0,  NULL)) != SQLITE_OK)
    {
        log_error(self->log, "SQLITE delete record exception: (%d) %s", rc, sqlite3_errmsg(self->db));
        result = false;
    }

    pthread_mutex_unlock(&self->mutex);

    return result;
}


TableContent *
model_get_table_columns(
    Model * self
    , char * table)
{
    return model_db_read(
                self
                , "PRAGMA table_info(%s)"
                , table);
}


bool
model_check_cover_exists(
    Model * self
    , char * cover_code)
{
    char query[512] = {0};
    bool cover_exists = false;
    sqlite3_stmt *res;
    int rc;

    sprintf(
        query
        , "SELECT cover_code FROM covers WHERE cover_code='%s';"
        , cover_code);

    pthread_mutex_lock(&self->mutex);
    
    if ((rc = sqlite3_prepare_v2(self->db, query, -1, &res, NULL)) == SQLITE_OK)
    {
        if(sqlite3_step(res) == SQLITE_ROW)
            cover_exists = true;

        sqlite3_finalize(res);
    }

    pthread_mutex_unlock(&self->mutex);

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
    time_t t     = time(NULL);
    struct tm tm = *localtime(&t);

    return model_db_write(
            self
            , "INSERT INTO covers "
              "(cover_code, orientation, PA10B_date_time, PA10B_activator_left, PA10B_activator_right, spare) "
              "VALUES('%s', '%s', '%02d.%02d.%d*%02d:%02d', '%s', '%s', '%s');"
            , cover_code
            , orientation ? "left" : "right"
            , tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min
            , bottle1_batch
            , bottle2_batch
            , spare ? "YES" : "NO");
}


bool
model_set_cover_used(
    Model * self
    , char * cover_code)
{
    return model_db_write(
            self
            , "UPDATE covers SET "
              "used='YES'"
              "WHERE cover_code='%s';"
            , cover_code);
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
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    printf("order: %d\n", order);

    return model_db_write(
            self
            , "INSERT INTO %s "
              "(ID, frame_code, frame_order, PA30R_date_time, PA30R_Primer_207_SIKA, PA30R_Remover_208_SIKA, PA30R_rework)"
              " VALUES ('172%08d', '%s', '%08d', '%02d.%02d.%d*%02d:%02d', '%s', '%s', '0');" 
            , table
            , order
            , frame_code
            , order
            , tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min
            , primer_207_sika
            , remover_208_sika);
}


bool
model_write_environment_data(
    Model * self
    , float temperature
    , float humidity)
{
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    return model_db_write(
            self
            , "INSERT INTO enviroment "
              "VALUES('%02d.%02d.%d*%02d:%02d:%02d', '%02.2f', '%02.2f');"
            , tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec
            , temperature
            , humidity);
}


bool
model_pa30r_update_frame_rework(
    Model * self
    , char * table
    , char * frame_code
    , uint8_t rework)
{
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    return model_db_write(
            self
            , "UPDATE %s SET "
              "PA30R_rework='%d', PA30R_date_time='%02d.%02d.%d*%02d:%02d' "
              "WHERE frame_code='%s';"
            , table
            , rework
            , tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min
            , frame_code);
}


bool
model_pa60r_update_frame_rework(
    Model * self
    , char * table
    , char * frame_code
    , uint8_t rework)
{
    time_t t     = time(NULL);
    struct tm tm = *localtime(&t);

    return model_db_write(
            self
            , "UPDATE %s SET "
              "PA60R_rework='%d', PA60R_date_time='%02d.%02d.%d*%02d:%02d' "
              "WHERE frame_code='%s';"
            , table
            , rework
            , tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min
            , frame_code);
}


bool
model_pa60r_update_frame(
     Model * self
    , char * table
    , char * frame_code)
{
    time_t t     = time(NULL);
    struct tm tm = *localtime(&t);

    return model_db_write(
            self
            , "UPDATE %s SET "
              "PA60R_date_time='%02d.%02d.%d*%02d:%02d' "
              "WHERE frame_code='%s';"
            , table
            , tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min
            , frame_code);
}


bool
model_pa30b_update_frame(
     Model * self
    , char * frame_code
    , char * left_cover_code
    , char * right_cover_code)
{
    time_t t     = time(NULL);
    struct tm tm = *localtime(&t);

    return model_db_write(
            self
            , "UPDATE frame_582 SET "
              "PA30B_date_time='%02d.%02d.%d*%02d:%02d', left_cover_code='%s', right_cover_code='%s' "
              "WHERE frame_code='%s';"
            , tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min
            , left_cover_code
            , right_cover_code
            , frame_code);
}


static void
to_unix_path(char * path)
{
	while(*path != 0)
	{
		if(*path == '\\')
			*path = '/';

		path++;
	}
}


bool
model_generate_frame_csv(
    Model * self
    , char * csv_path
    , char * table
    , char * frame_code
    , int8_t temperature
    , int8_t humidity)
{
    int8_t mon;
    int8_t day;
    int16_t year;
    int8_t hour;
    int8_t min; 
    char file_name[256];
	char primer_batch[32];
    char primer_part_number[32];
    char primer_fill_date[32];
    char primer_fill_time[32];
    char primer_expiration_date[32];
    char primer_expiration_time[32];

    TableContent * content = 
        model_db_read(
            self
            , "SELECT * FROM %s WHERE frame_code='%s';"
            , table
            , frame_code);

    if(content == NULL || content->super.size == 0)
    {
        log_error(self->log, "NULL pointer record for csv %s:%s",table, frame_code);  
        return false;
    }

    if(strcmp(table, "frame_582") == 0)
    {
		sscanf(content->array[0]->array[9], "%hhd.%hhd.%hd*%hhd:%hhd", &day, &mon, &year, &hour, &min);
        printf("%s\n", content->array[0]->array[9]);
    }
	else
		sscanf(content->array[0]->array[4], "%hhd.%hhd.%hd*%hhd:%hhd", &day, &mon, &year, &hour, &min);

     sscanf(
        content->array[0]->array[5]
        , "%[^*]*%[^*]*%[^ ] %[^*]*%[^*]*%[^*]*S"
        , primer_batch
        , primer_part_number
        , primer_expiration_date
        , primer_expiration_time
        , primer_fill_date
        , primer_fill_time);

    /*
    ** replace backslash to forward slash
    */
    to_unix_path(csv_path);

    snprintf(
		file_name
		, 255
		, "%s/172_%02d%02d%d%02d%02d00.csv"
		, csv_path
		, year
		, mon
		, day
		, hour
		, min);

    FILE * f = fopen(file_name, "r");

    if(f != NULL)
    {
    	log_error(self->log, "Can't write csv, file '%s' already exists", file_name);

    	array_delete(ARRAY(content));
    	fclose(f);
    	return false;
    }

    f = fopen(file_name, "w");

    if(f == NULL)
    {
        log_error(self->log, "Can't open file for write csv:  %s", file_name);
        array_delete(ARRAY(content));
        return false;
    }

    for(uint16_t i = 1; i <= 2578; i++)
    {
        switch(i)
        {
            case 1:
				fprintf(f, "%02d%02d%02d", year, mon, day);
                break;
            case 2:
				fprintf(f, "%02d%02d00", hour, min);
                break;
            case 3:
                fputs("00000172", f);
                break;
            case 4:
                fputs("00000172", f);
                break;
            case 5:
                if(strcmp(table, "frame_581") == 0)
                    fputs("1762975A", f);
                else if (strcmp(table, "frame_582") == 0)
                    fputs("1762976A", f);
                else if(strcmp(table, "frame_AU326_1") == 0)
                    fputs("1768362A", f);
                break;
            case 6:
                //frame ID
                fprintf(f, "%s", content->array[0]->array[0]);
                break;
            case 7:
                fputs("OK", f);
                break;
            case 9:
                fputs("1", f);
                break;
            case 10:
                fputs("DA", f);
                break;        
            case 8:
                fputs("200", f);
                break;
            case 1263:
                 // frame_code
                fprintf(f, "%s", content->array[0]->array[1]);
                break;
            case 1268:
                 if(strcmp(table, "frame_581") == 0)
                        fprintf(f, "AU581*1762975*%s*%s", content->array[0]->array[4], content->array[0]->array[2]);
                 else if(strcmp(table, "frame_582") == 0)
                        fprintf(f, "AU582*1762976*%s*%s", content->array[0]->array[9], content->array[0]->array[2]);
                 else if(strcmp(table, "frame_AU326_1") == 0)
                        fprintf(f, "AU326-1*1768362*%s*%s",content->array[0]->array[4], content->array[0]->array[2]);
                break;
            case 1288:
                //left cover code
                if(strcmp(table, "frame_582") == 0)
                     fprintf(f, "%s", content->array[0]->array[10]);
               break;
            case 1273:
                //right cover code
                if (strcmp(table, "frame_582") == 0)
                     fprintf(f, "%s", content->array[0]->array[11]);
                break;
            case 1278:
                // glue application date time (PA30B produce date time)
                if (strcmp(table, "frame_582") == 0)
                    fprintf(f, "%s", content->array[0]->array[9]);
                break;
            case 1283:
                if (strcmp(table, "frame_582") == 0)
                    fputs("0", f);
                break;
            case 265:
                fprintf(f, "%02d", temperature);
                break;
            case 269:
                fprintf(f, "%02d", humidity);
                break;
            case 1333:
                fputs("0", f);
                break;
            case 2553:
                fputs(primer_batch, f);
                break;
            case 2548:
                fputs(primer_part_number, f);
                break;
            case 2563:
                fputs(primer_fill_date, f);
                break;
            case 2568:
                fputs(primer_fill_time, f);
                break;
            case 2573:
                fputs(primer_expiration_date, f);
                break;
            case 2578:
                fputs(primer_expiration_time, f);
                break;
        }
        
        fputc(',', f);
        fflush(f);
    }
    
    fclose(f);
    array_delete(ARRAY(content));

    return true;
}


bool
model_supplier_bottle_exists(
    Model * self
    , char * bottle_batch_code)
{
    char query[512] = {0};

    sprintf(
        query
        , "SELECT batch_code FROM suplier_bottle WHERE batch_code='%s';"
        , bottle_batch_code);

    bool cover_exists = false;
    sqlite3_stmt * res = NULL;

    pthread_mutex_lock(&self->mutex);

    int rc = sqlite3_prepare_v2(self->db, query, -1, &res, NULL);
    
    if (rc == SQLITE_OK)
    {
        if(sqlite3_step(res) == SQLITE_ROW)
            cover_exists = true;

        sqlite3_finalize(res);
    }

    pthread_mutex_unlock(&self->mutex);

    return cover_exists;
}


bool
model_write_supplier_bottle(
    Model * self
    , char * chemical_type
    , char * batch_code)
{
    time_t t     = time(NULL);
    struct tm tm = *localtime(&t);

    return model_db_write(
            self
            , "INSERT INTO suplier_bottle "
              "VALUES ('%s', '%s', '*', '%02d.%02d.%d*%02d:%02d', '0');"
            , batch_code
            , chemical_type
            , tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min);
}


TableContent *
model_read_supplier_bottle(
    Model * self
    , char * batch_code)
{
    return model_db_read(
            self
            , "SELECT chemical_type, last_shake, fill_counter FROM suplier_bottle WHERE batch_code='%s';"
            , batch_code);
}


bool
model_write_glue_barrel(
    Model * self
    , char * material_description
    , char * material_type
    , char * batch_code
    , char * validity)
{
    return model_db_write(
            self
            , "INSERT INTO glue VALUES ('%s', '%s', '%s', '%s');"
            , material_description
            , material_type
            , batch_code
            , validity);
}


TableContent *
model_read_cover(
    Model * self
    , char * cover_code)
{
    return model_db_read(
            self
            , "SELECT orientation, PA10B_date_time, used, spare FROM covers WHERE cover_code='%s';"
            , cover_code);
}


TableContent *
model_read_frame(
    Model * self
    , char * frame_code)
{
    return model_db_read(
            self
            , "SELECT PA30R_date_time, PA60R_date_time, frame_order FROM frame_582 WHERE frame_code='%s';"
            , frame_code);
}


bool
model_update_bottle_shake_time(
    Model * self
    , char * batch_code)
{
    time_t t     = time(NULL);
    struct tm tm = *localtime(&t);

    return model_db_write(
            self
            , "UPDATE suplier_bottle SET "
              "last_shake='%02d.%02d.%d*%02d:%02d' "
              "WHERE batch_code='%s';"
            , tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min
            , batch_code);
}


bool
model_update_bottle_fill_counter(
    Model * self
    , char * batch_code
    , uint8_t fill_counter)
{
    return model_db_write(
            self
            , "UPDATE suplier_bottle SET "
              "fill_counter='%d' "
              "WHERE batch_code='%s';"
            , fill_counter
            , batch_code);
}


void
model_delete(Model * self)
{
    if(self != NULL)
    {
        sqlite3_close(self->db);
        log_delete(self->log);

        pthread_mutex_destroy(&self->mutex);
    }
}



