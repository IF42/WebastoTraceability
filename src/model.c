#include "model.h"
#include "version.h"


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


bool
model_db_callback_read(
	Model * self
	, int (*callback)(void *, int, char **, char **)
	, void * param 
    , const char * format
	, ...)
	
{
	char sql[1024];
	va_list args;
	va_start(args, format);

	vsnprintf(sql, 1023, format, args);

	pthread_mutex_lock(&self->mutex);
	int result = sqlite3_exec(self->db, sql, callback, param, NULL);
	pthread_mutex_unlock(&self->mutex);

	va_end(args);

	if(result == SQLITE_OK)
		return true;
	else
		return false;
}


bool
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


typedef struct
{
	char * csv_path;
	uint8_t temperature;
	uint8_t humidity;
}MES_CSV_Params;


static int 
__write_mes_csv_frame_582_callback(
	void * data
	, int argc
	, char ** argv
	, char ** col_name)
{
	(void) argc;
	(void) col_name;

    int8_t mon;
    int8_t day;
    int16_t year;
    int8_t hour;
    int8_t min; 
	char primer_batch[32];
    char primer_part_number[32];
    char primer_fill_date[32];
    char primer_fill_time[32];
    char primer_expiration_date[32];
    char primer_expiration_time[32];

    char file_name[256];
	MES_CSV_Params * params = data;

 	sscanf(
		argv[9]
		, "%hhd.%hhd.%hd*%hhd:%hhd"
		, &day, &mon, &year, &hour, &min);

    sscanf(
        argv[5]
        , "%[^*]*%[^*]*%[^ ] %[^*]*%[^*]*%[^*]*S"
        , primer_batch
        , primer_part_number
        , primer_expiration_date
        , primer_expiration_time
        , primer_fill_date
        , primer_fill_time);

    snprintf(
		file_name
		, 255
		, "%s/172_%02d%02d%d%02d%02d00.csv"
		, params->csv_path
		, year
		, mon
		, day
		, hour
		, min);

    FILE * f = fopen(file_name, "w");

	if(f == NULL) 
	{
		printf("cant open path: %s\n", file_name);
		return -1;
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
				fputs("1762976A", f);
                break;
            case 6:
                //frame ID
                fprintf(f, "%s", argv[0]);
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
                fprintf(f, "%s", argv[1]);
                break;
            case 1268:
				fprintf(f, "AU582*1762976*%s*%s", argv[9], argv[2]);
                break;
            case 1288:
                //left cover code
				 fprintf(f, "%s", argv[10]);
               break;
            case 1273:
                //right cover code
				 fprintf(f, "%s", argv[11]);
                break;
            case 1278:
                // glue application date time (PA30B produce date time)
				fprintf(f, "%s", argv[9]);
                break;
            case 1283:
				fputs("0", f);
                break;
            case 265:
                fprintf(f, "%02d", params->temperature);
                break;
            case 269:
                fprintf(f, "%02d", params->humidity);
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
    }

	fclose(f);

	return 0;
}


static int 
__write_mes_csv_frame_581_callback(
	void * data
	, int argc
	, char ** argv
	, char ** col_name)
{
	(void) argc;
	(void) col_name;

    int8_t mon;
    int8_t day;
    int16_t year;
    int8_t hour;
    int8_t min; 
	char primer_batch[32];
    char primer_part_number[32];
    char primer_fill_date[32];
    char primer_fill_time[32];
    char primer_expiration_date[32];
    char primer_expiration_time[32];

    char file_name[256];
	MES_CSV_Params * params = data;

 	sscanf(
		argv[9]
		, "%hhd.%hhd.%hd*%hhd:%hhd"
		, &day, &mon, &year, &hour, &min);

    sscanf(
        argv[5]
        , "%[^*]*%[^*]*%[^ ] %[^*]*%[^*]*%[^*]*S"
        , primer_batch
        , primer_part_number
        , primer_expiration_date
        , primer_expiration_time
        , primer_fill_date
        , primer_fill_time);

    snprintf(
		file_name
		, 255
		, "%s/172_%02d%02d%d%02d%02d00.csv"
		, (char*) params->csv_path
		, year
		, mon
		, day
		, hour
		, min);

    FILE * f = fopen(file_name, "w");

	if(f == NULL) 
	{

		return -1;
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
				fputs("1762975A", f);
                break;
            case 6:
                //frame ID
                fprintf(f, "%s", argv[0]);
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
                fprintf(f, "%s", argv[1]);
                break;
            case 1268:
				fprintf(f, "AU581*1762975*%s*%s", argv[4], argv[2]);
                break;
            case 265:
                fprintf(f, "%02d", params->temperature);
                break;
            case 269:
                fprintf(f, "%02d", params->humidity);
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
    }

	fclose(f);

	return 0;
}


static int 
__write_mes_csv_frame_AU326_callback(
	void * data
	, int argc
	, char ** argv
	, char ** col_name)
{
	(void) argc;
	(void) col_name;

    int8_t mon;
    int8_t day;
    int16_t year;
    int8_t hour;
    int8_t min; 
	char primer_batch[32];
    char primer_part_number[32];
    char primer_fill_date[32];
    char primer_fill_time[32];
    char primer_expiration_date[32];
    char primer_expiration_time[32];

    char file_name[256];
	MES_CSV_Params * params = data;

	sscanf(
		argv[4]
		, "%hhd.%hhd.%hd*%hhd:%hhd"
		, &day, &mon, &year, &hour, &min);

    sscanf(
        argv[5]
        , "%[^*]*%[^*]*%[^ ] %[^*]*%[^*]*%[^*]*S"
        , primer_batch
        , primer_part_number
        , primer_expiration_date
        , primer_expiration_time
        , primer_fill_date
        , primer_fill_time);

    snprintf(
		file_name
		, 255
		, "%s/172_%02d%02d%d%02d%02d00.csv"
		, params->csv_path
		, year
		, mon
		, day
		, hour
		, min);

    FILE * f = fopen(file_name, "w");

	if(f == NULL) 
		return -1;

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
                fputs("1768362A", f);
                break;
            case 6:
                //frame ID
                fprintf(f, "%s", argv[0]);
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
                fprintf(f, "%s", argv[1]);
                break;
            case 1268:
                fprintf(f, "AU326-1*1768362*%s*%s", argv[4], argv[2]);
                break;
            case 265:
                fprintf(f, "%02d", params->temperature);
                break;
            case 269:
                fprintf(f, "%02d", params->humidity);
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
    }

	fclose(f);

	return 0;
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
model_export_mes_csv(
    Model * self
    , char * csv_path
    , char * table
    , char * frame_code
    , int8_t temperature
    , int8_t humidity)
{
    to_unix_path(csv_path);

	MES_CSV_Params params = {csv_path, temperature, humidity};

    if(strcmp(table, "frame_581") == 0)
	{
		return model_db_callback_read(
					self
					, __write_mes_csv_frame_581_callback
					, &params, "SELECT * FROM %s WHERE frame_code='%s';"
            		, table
            		, frame_code);
	}
    else if(strcmp(table, "frame_582") == 0)
	{
		return model_db_callback_read(
					self
					, __write_mes_csv_frame_582_callback
					, &params, "SELECT * FROM %s WHERE frame_code='%s';"
            		, table
            		, frame_code);
	}
    else if(strcmp(table, "frame_AU326_1") == 0)
	{
		return model_db_callback_read(
					self
					, __write_mes_csv_frame_AU326_callback
					, &params, "SELECT * FROM %s WHERE frame_code='%s';"
            		, table
            		, frame_code);
	}
	else
		return false;
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



