/* Copyright (c), bowtoes (bow.toes@mailfence.com)
Apache 2.0 license, http://www.apache.org/licenses/LICENSE-2.0
Full license can be found in 'license' file */

#include <errno.h>
#include <string.h>
#include <stdio.h>

#include <brrtools/brrnum.h>
#include <brrtools/brrmacro.h>

#include "neinput.h"
#include "nelog.h"
#include "process.h"

#define _log_stat(_stat_, _style_, ...) do {\
    if ((_stat_).assigned) \
		Nor(,"    (!f=m:%*i / %*i!) (!" _style_ ":%s!)", input_count_digits, (_stat_).succeeded, input_count_digits, (_stat_).assigned, __VA_ARGS__);\
} while (0)
static inline int
i_print_report(const nestate_t *const state)
{
	brrsz input_count_digits = 1 + brrnum_udigits(state->n_inputs, 10);
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
	Nor(,"Successfully processed a total of (!f=m:%*i / %*i inputs!)", input_count_digits, total_success, input_count_digits, state->n_inputs);
	if (state->cfg.full_report) {
		_log_stat(state->stats.oggs, "f=b", "Regrained Oggs");
		_log_stat(state->stats.wems, "f=g", "Converted WwRIFFs");
		_log_stat(state->stats.wsps, "f=y", "Processed wsp's");
		_log_stat(state->stats.bnks, "f=r", "Processed bnk's");
		_log_stat(state->stats.wem_extracts, "f=g", "Extracted WwRIFFs");
		_log_stat(state->stats.wem_converts, "f=b", "Auto-converted WwRIFFs");
	}
	return 0;
}
#undef _log_stat

int
main(int argc, char **argv)
{
	static nestate_t state = {
		.default_input = {
			.library_index = -1,
			.log_label = logpri_normal,
			.cfg = {
				.log_enabled = 1,
				.log_color_enabled = 1,
		#if defined(Ne_debug)
				.log_debug = 1,
		#endif
				.add_comments = 1,
			},
		},
		.cfg = {
			.should_reset = 0,
			.log_style_enabled = 1,
			.report_card = 1,
			.full_report = 1,
		},
	};

	if (nelog_init(1)) {
		fprintf(stderr, "Failed to initialize logging output : %s", strerror(errno));
		return errno;
	}
	if (argc == 1) {
		print_usage();
	} else if (nestate_init(&state, argc - 1, argv + 1)) {
		fprintf(stderr, "Failed to take inputs : %s", strerror(errno));
		return errno;
	}

	brrlog_maxlabel(state.default_input.cfg.log_debug ? logpri_debug : state.default_input.log_label);
	if (!state.n_inputs) {
		Err(,"No files passed, exiting.");
		brrlog_deinit();
		return 1;
	}

	neprocess_inputs(&state);
	nestate_clear(&state);

	if (state.cfg.report_card || state.cfg.full_report)
		i_print_report(&state);

	brrlog_deinit();
	return 0;
}
