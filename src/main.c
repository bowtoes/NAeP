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
#include <brrtools/brrnum.h>

#include "neinput.h"
#include "process.h"

static inline int
i_print_report(const nestate_t *const state)
{
	brrsz input_count_digits = 1 + brrnum_ndigits(state->n_inputs, 10, 0);
	brrsz total_success =
	      state->stats.oggs.succeeded
	    + state->stats.wems.succeeded
	    + state->stats.wsps.succeeded
	    + state->stats.bnks.succeeded;
	brrsz total_failure =
	      state->stats.oggs.failed
	    + state->stats.wems.failed
	    + state->stats.wsps.failed
	    + state->stats.bnks.failed;
	Style(n, normal, "Successfully processed a total of ");
	Style(np,extra_info , "%*i / %*i", input_count_digits, total_success, input_count_digits, state->n_inputs);
	Lst(p," inputs");
	if (state->settings.full_report) {
		#define _log_stat(_stat_, _type_, _msg_) do {\
		    if ((_stat_).assigned) {\
		        Style(n,normal, "    ");\
		        Style(np,extra_info, "%*i / %*i", input_count_digits, (_stat_).succeeded, input_count_digits, (_stat_).assigned);\
		        Style(p,_type_, " "_msg_);\
		    }\
		} while (0)
		_log_stat(state->stats.oggs, ft_ogg, "Regrained Oggs");
		_log_stat(state->stats.wems, ft_wem, "Converted WwRIFFs");
		_log_stat(state->stats.wsps, ft_wsp, "Processed wsp's");
		_log_stat(state->stats.bnks, ft_bnk, "Processed bnk's");
		_log_stat(state->stats.wem_extracts, ft_wem, "Extracted WwRIFFs");
		_log_stat(state->stats.wem_converts, ft_ogg, "Auto-converted WwRIFFs");
	}
	return 0;
}

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
				.add_comments = 1,
			},
		},
		.settings = {
			.should_reset = 0,
			.log_style_enabled = 1,
			.report_card = 1,
			.full_report = 1,
		},
	};

	if (brrlog_set_max_log(0)) {
		fprintf(stderr, "Failed to initialize logging output : %s", strerror(errno));
		return errno;
	}
	if (argc == 1) {
		print_usage();
	} else if (nestate_init(&state, argc - 1, argv + 1)) {
		fprintf(stderr, "Failed to take inputs : %s", strerror(errno));
		return errno;
	}

	{
		gbrrlogctl.flush_enabled = 1;
		gbrrlogctl.flush_always = 1;
		gbrrlog_level(critical).prefix = "[CRAZY] ";
		gbrrlog_level(error).prefix    = "[ERROR] ";
		gbrrlog_level(warning).prefix  = "[CAUTION] ";
		gbrrlog_level(debug).prefix    = "[DEBUG] ";
		gbrrlog_format(debug) = BRRLOG_FORMAT_FORE(brrlog_color_green);
	}

	brrlog_set_max_priority(state.default_input.flag.log_debug ? brrlog_priority_debug : state.default_input.log_priority);
	if (!state.n_inputs) {
		BRRLOG_ERR("No files passed");
		return 1;
	}

	neprocess_inputs(&state);
	nestate_clear(&state);

	if (state.settings.report_card || state.settings.full_report)
		i_print_report(&state);

	brrlog_deinit();
	return 0;
}
