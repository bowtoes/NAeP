#include "process.h"

#include <stdlib.h>
#include <string.h>

#include "lib.h"
#include "print.h"

static inline int
i_process_arc(nestate_t *const state, const neinput_t *const input)
{

}

int
neprocess_wsp(nestate_t *const state, const neinput_t *const input)
{
	int err = 0;
	state->stats.wsps.assigned++;
	if (input->flag.dry_run) {
		LOG_FORMAT(LOG_PARAMS_DRY, "Process Archive (dry) ");
	} else {
		LOG_FORMAT(LOG_PARAMS_WET, "Processing Archive... ");
		err = i_process_arc(state, input);
	}
	if (!err) {
		state->stats.wsps.succeeded++;
		LOG_FORMAT(LOG_PARAMS_SUCCESS, "Success!\n");
	} else {
		state->stats.wsps.failed++;
		LOG_FORMAT(LOG_PARAMS_FAILURE, "Failure! (%d)\n", err);
	}
	return err;
}
