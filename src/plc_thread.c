#include "plc_thread.h"


bool
plc_thread_run(
		PLC_Thread * self
		, Model * model
		, S7Object client)
{
    return self->callback(self, model, client);
}


char *
cli_error(int cli_result)
{
	static char error_buffer[128]= {0};

	Cli_ErrorText(cli_result, error_buffer, sizeof(error_buffer));
	return error_buffer;
}



void
plc_thread_delete(PLC_Thread * self)
{
    self->delete(self);
}
