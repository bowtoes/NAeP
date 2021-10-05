/*
Copyright 2021 BowToes (bow.toes@mailfence.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <brrtools/brrlog.h>

#include "input.h"
#include "print.h"
#include "process.h"

static void
i_init_brrlog(void)
{
	if (brrlog_setlogmax(0)) {
		fprintf(stderr, "Failed to initialize logging output : %s", strerror(errno));
		exit(1);
	}
	gbrrlogctl.flush_enabled = 1;
	gbrrlogctl.flush_always = 1;
	gbrrlog_level_critical.prefix = "[CRAZY] ";
	gbrrlog_level_error.prefix    = "[ERROR] ";
	gbrrlog_level_warning.prefix  = "[CAUTION] ";
	gbrrlog_level_debug.prefix    = "[DEBUG] ";
	gbrrlog_format_debug = BRRLOG_FORMAT_FORE(brrlog_color_green);
}

int
main(int argc, char **argv)
{
	static const neinputT default_input = {
		.library_index = -1,
		.log_enabled = 1,
		.log_color_enabled = 1,
		.log_priority = brrlog_priority_normal,
#if defined(NeDEBUG)
		.log_debug = 1,
#endif
	};
	static nestateT state = {
		.should_reset = 0,
		.log_style_enabled = 1,
		.report_card = 1,
		.full_report = 1,
	};
	neinputT *inputs = NULL;
	neinput_libraryT *libraries = NULL;
	int err = 0;

	if (argc == 1) {
		print_usage();
	} else if (neinput_take_inputs(&state, &libraries, &inputs, default_input, argc - 1, argv + 1)) {
		fprintf(stderr, "Failed to take inputs : %s", strerror(errno));
		return 1;
	}

	i_init_brrlog();
	if (state.n_inputs) {
		if (neprocess_inputs(&state, libraries, inputs))
			err = 1;
	} else {
		err = 1;
		brrlog_setmaxpriority(default_input.log_debug?brrlog_priority_debug:default_input.log_priority);
		BRRLOG_ERR("No files passed");
	}
	neinput_clear_all(&state, &libraries, &inputs);

	if (state.report_card || state.full_report)
		print_report(&state);
	brrlog_deinit();
	return err;
}
