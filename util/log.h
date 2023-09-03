/**
** @file log.h
** @brief Header file for log library
** @author Petr Horáček
*/

#ifndef _LOG_H_
#define _LOG_H_

#include <stdio.h>
#include <stdbool.h>


/**
** @brief Log structure declaration
*/
struct Log;


/**
** @brief Log structure alias
*/
typedef struct Log Log;


/**
** @brief Log object constructor. Log messages are writen into all given FILE streams.
** @param f_list_length Number of FILE pointer references 
** @param f_list List FILE pointer references 
** @return Initialized object of Log structure. It is necessary 
** to call log_delete function for release memory
*/
Log *
log_new(
    size_t length
    , FILE * f_list[length]);


/**
** @brief Function for writing debug log messages into predefined FILE streams
** @param log Pointer to Log object
** @param format String message with predefined printf format
** @param ... Optional parameters defined in input format
*/
void
log_debug(
    Log * log
    , char * format
    , ...);


/**
** @brief Function for writing waring log messages into predefined FILE streams
** @param log Pointer to Log object
** @param format String message with predefined printf format
** @param ... Optional parameters defined in input format
*/
void
log_warning(
    Log * log
    , char * format
    , ...);



/**
** @brief Function for writing error log messages into predefined FILE streams
** @param log Pointer to Log object
** @param format String message with predefined printf format
** @param ... Optional parameters defined in input format
*/
void
log_error(
    Log * log
    , char * format
    , ...);


/**
** @brief Settup function for control of writing log messages into predefined FILE streams
** @param log Pointer to Log object
** @param enable Control value for log messages output. True = log disable.
*/
void 
log_set_quiet(
    Log * log
    , bool enable);


/**
** @brief Function for getting information about enabled logging
** @param log Pointer to Log object
** @return Information about enabled logging
*/
bool
log_is_quiet(Log * log);


/**
** @brief Prepare log file name with current time stamp and input name
** Output char array is pointer to static array and must not be released
*/
const char * 
log_prepare_filename(char * prefix);


/**
** @brief Destructor for Log object
** @param log Pointer to Log object
*/
void
log_delete(Log * log);


#endif 

