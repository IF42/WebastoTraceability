/**
** @file log.c
** @brief Logging library
** @author Petr Horáček
*/

#include "log.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <stdarg.h>


struct Log
{
    pthread_mutex_t lock;
    bool quiet;

    size_t length;
    FILE * stream[];
};


/**
** @brief Log level constants
*/
typedef enum
{
    Debug
    , Warning
    , Error
    , LogLevelNumber
}LogLevel;


/**
** @brief Log level string labels used in logging functions
*/
const char * log_level_label[LogLevelNumber] =
{
    "Debug"
    , "Warning"
    , "Error"
};


static char *
_log_time_stamp()
{
    static char str_time[128] = {0};
    time_t raw_time;
    struct tm * timeinfo;

    time(&raw_time);
    timeinfo = localtime(&raw_time);

    snprintf(
        str_time
        , 128
        , "%02d-%02d-%d %02d:%02d:%02d"
        , timeinfo->tm_mday
        , timeinfo->tm_mon + 1
        , timeinfo->tm_year + 1900
        , timeinfo->tm_hour
        , timeinfo->tm_min
        , timeinfo->tm_sec);

    return str_time;
}


static void
_log_process(
    Log * log
    , const char * log_level
    , const char * format
    , va_list args)
{
    pthread_mutex_lock(&log->lock);

    for(size_t i = 0; i < log->length; i++)
    {
        //va_list args_cpy;
        //va_copy(args_cpy, *args);

        fprintf(log->stream[i], "%s [%s] - ", _log_time_stamp(), log_level);
        vfprintf(log->stream[i], format, args);
        fprintf(log->stream[i], "\n");

        fflush(log->stream[i]);
    }

    pthread_mutex_unlock(&log->lock);
}




Log *
log_new(
    size_t length
    , FILE * stream[length])
{
    size_t memsize = (sizeof(FILE*) * length);

    Log * log = malloc(sizeof(Log) + memsize);
    
    if(log != NULL)
    {
        memcpy(log->stream, stream, memsize);
        log->quiet = false;
        log->length = length;

        if (pthread_mutex_init(&log->lock, NULL) != 0)
        {
            free(log);
            return NULL;
        }

        log_debug(log, "Log init success");
    }

    return log;
}


void
log_debug(
    Log * log
    , char * format
    , ...)
{
    if(log->quiet == false)
    {
        va_list args;
        va_start(args, format);

        _log_process(log, log_level_label[Debug], format, args);

        va_end(args);
    }
}


void
log_warning(
    Log * log
    , char * format
    , ...)
{
    if(log->quiet == false)
    {
        va_list args;
        va_start(args, format);

        _log_process(log, log_level_label[Warning], format, args);

        va_end(args);
    }
}


void
log_error(
    Log * log
    , char * format
    , ...)
{
    if(log->quiet == false)
    {
        va_list args;
        va_start(args, format);

        _log_process(log, log_level_label[Error], format, args);

        va_end(args);
    }
}



void 
log_set_quiet(
    Log * log
    , bool enable)
{
    log->quiet = enable;
}


bool
log_is_quiet(Log * log)
{
    return log->quiet;
}


const char * 
log_prepare_filename(char * prefix)
{
    static char log_filename[256] = {0};
    time_t rawtime;
    struct tm * timeinfo;
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    snprintf(
        log_filename
        , 255
        , "%s_%d_%d_%d_%d_%d_%d.log"
        , prefix,timeinfo->tm_mday
        , timeinfo->tm_mon + 1
        , timeinfo->tm_year + 1900
        , timeinfo->tm_hour
        , timeinfo->tm_min
        , timeinfo->tm_sec);

    return log_filename;
}


void
log_delete(Log * log)
{
    if(log != NULL)
    {
        pthread_mutex_destroy(&log->lock);
        free(log);
    }
}



