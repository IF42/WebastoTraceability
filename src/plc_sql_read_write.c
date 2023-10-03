#include "plc_sql_read_write.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>


typedef enum
{
	INIT
    , WAIT
	, WRITE
    , FINISH
    , ERROR
	, STATE_N
}ThreadStep;


#define THREAD_STEP(T)				\
	T == INIT ? "STEP_INIT":		\
	T == WAIT ? "STEP_WAIT":		\
	T == WRITE ? "STEP_WRITE":		\
	T == FINISH ? "STEP_FINISH":	\
	T == ERROR ? "STEP_ERROR":		\
				"STEP_UNKNOWN"


typedef struct
{
    bool is_error;
    ThreadStep step;
}ThreadResult;


#define throw_error return (ThreadResult){.is_error = true}
#define state(T) (ThreadResult){.step=T}


typedef struct SQL_ReadWrite
{
	PLC_Thread super;

	uint8_t db_index;

	char input_buffer[1024];
	char output_buffer[1028];

	ThreadStep step;
	ThreadResult (*thread_state[STATE_N])(struct SQL_ReadWrite *, Model *, S7Object);
}SQL_ReadWrite;


#define SQL_ReadWrite(...)(SQL_ReadWrite){__VA_ARGS__}
#define SQL_READ_WRITE(T)((SQL_ReadWrite*) T)


static ThreadResult
__init(
	SQL_ReadWrite * self
	, Model * model
	, S7Object client)
{
	int cli_result;
	uint8_t result[4] = {0};

	/*
	** Initialization of output header (NDR, ERROR, OUT_LEN)
	*/
	if((cli_result = Cli_DBWrite(
						client
						, self->db_index
						, 1028
						, sizeof(result)
						, result)) != 0)
	{
		log_error(
			model->log
			, "%s:%d: RW DB(%d): %s"
			, __FILE__
			, __LINE__
			, self->db_index
			, cli_error(cli_result));

		throw_error;
	}
	else
	{
		memset(self->output_buffer, 0, sizeof(self->output_buffer));
		return state(WAIT);
	}
}


static int
__load_database_data(
	void * buffer
	, int argc
	, char ** argv
	, char ** column_name)
{
    (void) column_name;
	uint16_t index = 4;

	for(int arg_index = 0; arg_index < argc; arg_index++)
	{
		size_t arg_length = strlen(argv[arg_index]);

		if(arg_index != 0)
		{
			((char*) buffer)[index] = '\0';
			index++;
		}

		for(size_t i = 0; i < arg_length && index < 1028; index++, i++)
			((char*) buffer)[index] = argv[arg_index][i];
	}

	// OUT_LEN
	((char*) buffer)[2] = (index - 4) >> 8;
	((char*) buffer)[3] = (index - 4);

	return 0;
}


static ThreadResult
__wait(
	SQL_ReadWrite * self
	, Model * model
	, S7Object client)
{
	int cli_result;
	uint8_t header[4];

	/*
	** Reading first 4 bytes with Req status and size to read
	*/
	if((cli_result = Cli_DBRead(
						client
						, self->db_index
						, 0
						, sizeof(header)
						, header)) != 0)
	{
		log_error(
			model->log
			, "%s:%d: RW DB(%d): %s"
			, __FILE__
			, __LINE__
			, self->db_index
			, cli_error(cli_result));

		throw_error;
	}

	/*
	** if the Req status is false
	*/
	if((header[0] & 0x01) == false)
		return state(WAIT);

	/*
	** getting size to read for effective reading
	*/
	uint16_t size_to_read = (header[2] << 8) | header[3];

	/*
	** don't try to read zero or more than 1024 bytes (size of the input buffer)
	*/
	if(size_to_read > 1024)
	{
		log_debug(
			model->log
			, "%s:%d: RW DB(%d): size to read out of index: %d"
			, __FILE__
			, __LINE__
			, self->db_index
			, size_to_read);

		return state(ERROR);
	}
	else if(size_to_read == 0)
		return state(WAIT);

	/*
	** reading of input input buffer with stored SQL request
	*/
	if((cli_result = Cli_DBRead(
						client
						, self->db_index
						, sizeof(header)
						, size_to_read
						, self->input_buffer)) != 0)
	{
		log_error(
			model->log
			, "%s:%d: RW DB(%d): %s"
			, __FILE__
			, __LINE__
			, self->db_index
			, cli_error(cli_result));

		throw_error;
	}

	// set zero at the end of string (terminating character)
	self->input_buffer[size_to_read] = '\0';

	if(strncmp("SELECT", self->input_buffer, strlen("SELECT")) == 0)
	{
		if(model_db_callback_read(
                model
                , __load_database_data
                , self->output_buffer
				, self->input_buffer) == false)
        {
			return state(ERROR);
        }
	}
	else
	{
		if(model_db_write(model, self->input_buffer) == false)
			return state(ERROR);
	}

	return state(WRITE);
}


static ThreadResult
_write(
	SQL_ReadWrite * self
	, Model * model
	, S7Object client)
{
	int cli_result;

	// set NDR = true
	((char*) self->output_buffer)[0] = 0x01;

	/*
	** getting size to write for effective writing
	*/
	uint16_t size_to_write = 4 + ((self->output_buffer[3] << 8) | self->output_buffer[2]);

	if(size_to_write > 1024)
		size_to_write = 1024;

	/*
	** Writing error result
	*/
	if((cli_result = Cli_DBWrite(
						client
						, self->db_index
						, 1028
						, size_to_write
						, self->output_buffer)) != 0)
	{
		log_error(
			model->log
			, "%s:%d: RW DB(%d): %s"
			, __FILE__
			, __LINE__
			, self->db_index
			, cli_error(cli_result));

		throw_error;
	}

	return state(FINISH);
}


static ThreadResult
__finish(
	SQL_ReadWrite * self
	, Model * model
	, S7Object client)
{
	int cli_result;
	uint8_t header;

	/*
	** Reading first 4 bytes with Req status and size to read
	*/
	if((cli_result = Cli_DBRead(
						client
						, self->db_index
						, 0
						, sizeof(header)
						, &header)) != 0)
	{
		log_error(
			model->log
			, "%s:%d: RW DB(%d): %s"
			, __FILE__
			, __LINE__
			, self->db_index
			, cli_error(cli_result));

		throw_error;
	}

	/*
	** if the Req status is false
	*/
	if((header & 0x01) == false)
		return state(INIT);
	else
		return state(FINISH);
}

static ThreadResult
__error(
	SQL_ReadWrite * self
	, Model * model
	, S7Object client)
{
	int cli_result;
	uint8_t header;

	// NDR = false, ERROR = true, IN_LEN = 0
	uint8_t result[4] = {0x02, 0, 0, 0};

	/*
	** Writing error result
	*/
	if((cli_result = Cli_DBWrite(
						client
						, self->db_index
						, 1028
						, sizeof(result)
						, result)) != 0)
	{
		log_error(
			model->log
			, "%s:%d: RW DB(%d): %s"
			, __FILE__
			, __LINE__
			, self->db_index
			, cli_error(cli_result));

		throw_error;
	}

	/*
	** Reading first 4 bytes with Req status and size to read
	*/
	if((cli_result = Cli_DBRead(
						client
						, self->db_index
						, 0
						, sizeof(header)
						, &header)) != 0)
	{
		log_error(
			model->log
			, "%s:%d: RW DB(%d): %s"
			, __FILE__
			, __LINE__
			, self->db_index
			, cli_error(cli_result));

		throw_error;
	}

	/*
	** if the Req status is false
	*/
	if((header & 0x01) == false)
		return state(INIT);
	else
		return state(ERROR);
}


static bool
db_read_write_run(
	PLC_Thread * self
	, Model * model
	, S7Object client)
{
	if(SQL_READ_WRITE(self)->step < STATE_N)
	{
		/*
		** calling State Machine transition function and getting either
		** new state or error
		** if the result is error, then running of the State Machine is finished
		*/
		ThreadResult result =
			SQL_READ_WRITE(self)->thread_state[SQL_READ_WRITE(self)->step](
					SQL_READ_WRITE(self)
					, model
					, client);

		if(result.is_error == false)
			SQL_READ_WRITE(self)->step = result.step;
		else
			return false;
	}
	else
		SQL_READ_WRITE(self)->step = INIT;

	return true;
}


PLC_Thread *
plc_db_read_write_new(
	uint8_t db_index)
{
	SQL_ReadWrite * self = malloc(sizeof(SQL_ReadWrite));

	if(self != NULL)
	{
		*self = SQL_ReadWrite(
			.super = PLC_Thread(db_read_write_run, (void(*)(PLC_Thread*)) free)
			, .step = INIT
			, .db_index = db_index
			, .thread_state = {__init, __wait, _write, __finish, __error}
		);
	}

	return PLC_THREAD(self);
}


