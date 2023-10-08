#include "plc_mes_csv.h"

#include <stdlib.h>
#include <endian.h>


typedef enum
{
	INIT
    , WAIT
    , FINISH
    , ERROR
	, STATE_N
}ThreadStep;


typedef struct
{
    bool is_error;
    ThreadStep step;
}ThreadResult;


#define throw_error return (ThreadResult){.is_error = true}
#define state(T) (ThreadResult){.step=T}


typedef struct PLC_MES_csv
{
    PLC_Thread super;

    uint8_t db_index;

    ThreadStep step;
    ThreadResult (*thread_state[STATE_N])(struct PLC_MES_csv *, Model *, S7Object);
}PLC_MES_csv;


#define PLC_MES_csv(...)(PLC_MES_csv){__VA_ARGS__}
#define PLC_MES_CSV(T)((PLC_MES_csv*) T)


static ThreadResult
__init(
	PLC_MES_csv * self
	, Model * model
	, S7Object client)
{
	int cli_result;
	uint8_t result = 0;

	/*
	** Initialization of output header
	*/
	if((cli_result = Cli_DBWrite(
						client
						, self->db_index
						, 0
						, sizeof(result)
						, &result)) != 0)
	{
		log_error(
			model->log
			, "%s:%d: MES CSV export(%d): %s"
			, __FILE__
			, __LINE__
			, self->db_index
			, cli_error(cli_result));

		throw_error;
	}
	else
		return state(WAIT);
}


static ThreadResult
__wait(
	PLC_MES_csv * self
	, Model * model
	, S7Object client)
{
	int cli_result;
	uint8_t header;

	/*
	** Reading first 1 byte
	*/
	if((cli_result = Cli_DBRead(
						client
						, self->db_index
						, 2
						, sizeof(header)
						, &header)) != 0)
	{
		log_error(
			model->log
			, "%s:%d: MES CSV export(%d): %s"
			, __FILE__
			, __LINE__
			, self->db_index
			, cli_error(cli_result));

		throw_error;
	}



	/*
	** if the EXECUTE status is false
	*/
	if((header & 0x01) == true)
	{
		char buffer[772];

		if((cli_result = Cli_DBRead(
							client
							, self->db_index
							, 4
							, sizeof(buffer)
							, &buffer)) != 0)
		{
			log_error(
				model->log
				, "%s:%d: MES CSV export(%d): %s"
				, __FILE__
				, __LINE__
				, self->db_index
				, cli_error(cli_result));

			throw_error;
		}

		/*
		** set string ending zero, length of the string is stored in
		** second byte of PLC string type
		*/
		buffer[2 + buffer[1]]   = '\0';
		buffer[514 + buffer[513]] = '\0';
		buffer[258 + buffer[257]] = '\0';

		if(model_export_mes_csv(
		        model
		        , &buffer[514]
		        , &buffer[2]
		        , &buffer[258]
		        , ((float) ((buffer[768] << 8) | buffer[769]) /10.0)
		        , ((float) ((buffer[768] << 8) | buffer[769])) /10.0) == true)
		{
			return state(FINISH);
		}
		else
			return state(ERROR);
	}
	else
		return state(WAIT);
}



static ThreadResult
__finish(
	PLC_MES_csv * self
	, Model * model
	, S7Object client)
{
	int cli_result;
	uint8_t header;

	// DONE = true, ERROR = false
	uint8_t result = 0x01;

	/*
	** Writing DONE result
	*/
	if((cli_result = Cli_DBWrite(
						client
						, self->db_index
						, 0
						, sizeof(result)
						, &result)) != 0)
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
	** Reading first 1 byte
	*/
	if((cli_result = Cli_DBRead(
						client
						, self->db_index
						, 2
						, sizeof(header)
						, &header)) != 0)
	{
		log_error(
			model->log
			, "%s:%d: MES CSV export(%d): %s"
			, __FILE__
			, __LINE__
			, self->db_index
			, cli_error(cli_result));

		throw_error;
	}

	/*
	** if the EXECUTE status is false
	*/
	if((header & 0x01) == false)
		return state(INIT);
	else
		return state(FINISH);
}


static ThreadResult
__error(
	PLC_MES_csv * self
	, Model * model
	, S7Object client)
{
	int cli_result;
	uint8_t header;

	// DONE = false, ERROR = true
	uint8_t result = 0x02;

	/*
	** Writing ERROR result
	*/
	if((cli_result = Cli_DBWrite(
						client
						, self->db_index
						, 0
						, sizeof(result)
						, &result)) != 0)
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
	** Reading first 1 byte
	*/
	if((cli_result = Cli_DBRead(
						client
						, self->db_index
						, 2
						, sizeof(header)
						, &header)) != 0)
	{
		log_error(
			model->log
			, "%s:%d: MES CSV export(%d): %s"
			, __FILE__
			, __LINE__
			, self->db_index
			, cli_error(cli_result));

		throw_error;
	}

	/*
	** if the EXECUTE status is false
	*/
	if((header & 0x01) == false)
		return state(INIT);
	else
		return state(ERROR);
}


static bool
plc_mes_csv_run(
	PLC_Thread * self
	, Model * model
	, S7Object client)
{
	if(PLC_MES_CSV(self)->step < STATE_N)
	{
		/*
		** calling State Machine transition function and getting either
		** new state or error
		** if the result is error, then running of the State Machine is finished
		*/
		ThreadResult result =
				PLC_MES_CSV(self)->thread_state[PLC_MES_CSV(self)->step](
					PLC_MES_CSV(self)
					, model
					, client);

		if(result.is_error == false)
			PLC_MES_CSV(self)->step = result.step;
		else
			return false;
	}
	else
		PLC_MES_CSV(self)->step = INIT;

	return true;
}


PLC_Thread *
plc_mes_csv_new(
    uint8_t db_index)
{
	PLC_MES_csv * self = malloc(sizeof(PLC_MES_csv));

	if(self != NULL)
	{
		*self = (PLC_MES_csv)
		{
			.super      	= PLC_Thread(plc_mes_csv_run, (void(*)(PLC_Thread*)) free)
			, .step 		= INIT
			, .db_index 	= db_index
			, .thread_state = {__init, __wait, __finish, __error}
		};
	}

	return PLC_THREAD(self);
}



