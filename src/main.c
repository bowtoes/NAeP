/*
Copyright 2021-2022 BowToes (bow.toes@mailfence.com)

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

int
main(int argc, char **argv)
{
	static nestate_t state = {
		.default_input = {
			.library_index = -1,
			.log_priority = brrlog_priority_normal,
			.flag = {
				.log_enabled = 1,
				.log_color_enabled = 1,
		#if defined(Ne_debug)
				.log_debug = 1,
		#endif
			},
		},
		.settings = {
			.should_reset = 0,
			.log_style_enabled = 1,
			.report_card = 1,
			.full_report = 1,
		},
	};
	int err = 0;

	if (argc == 1) {
		print_usage();
	} else if (nestate_init(&state, argc - 1, argv + 1)) {
		fprintf(stderr, "Failed to take inputs : %s", strerror(errno));
		return errno;
	}

	{
		if (brrlog_set_max_log(0)) {
			fprintf(stderr, "Failed to initialize logging output : %s", strerror(errno));
			return errno;
		}
		gbrrlogctl.flush_enabled = 1;
		gbrrlogctl.flush_always = 1;
		gbrrlog_level(critical).prefix = "[CRAZY] ";
		gbrrlog_level(error).prefix    = "[ERROR] ";
		gbrrlog_level(warning).prefix  = "[CAUTION] ";
		gbrrlog_level(debug).prefix    = "[DEBUG] ";
		gbrrlog_format(debug) = BRRLOG_FORMAT_FORE(brrlog_color_green);
	}

	if (!state.n_inputs) {
		brrlog_set_max_priority(state.default_input.flag.log_debug?brrlog_priority_debug:state.default_input.log_priority);
		BRRLOG_ERR("No files passed");
		return 1;
	}
	neprocess_inputs(&state, state.libraries, state.inputs);
	nestate_clear(&state);

	if (state.settings.report_card || state.settings.full_report)
		print_report(&state);
	brrlog_deinit();
	return err;
}
