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

#include "process.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <brrtools/brrlog.h>
#include <brrtools/brrpath.h>

#include "lib.h"
#include "errors.h"
#include "print.h"

static inline void
i_set_log_state(const nestate_t *const state, const neinput_t *const input)
{
	if (state->settings.log_style_enabled)
		gbrrlogctl.style_disabled = !input->flag.log_color_enabled;
#if defined(Ne_debug)
	gbrrlogctl.debug_enabled = 1;
	brrlog_set_max_priority(brrlog_priority_debug);
#else
	gbrrlogctl.debug_enabled = input->flag.log_debug;
	brrlog_set_max_priority(input->log_priority);
#endif
}
static inline int
i_check_input(const neinput_t *const input)
{
	brrpath_stat_result_t stat;
	brrstringr_t path_str = brrstringr_cast(input->path);
	if (brrpath_stat(&stat, &path_str)) {
		BRRLOG_ERR("Failed to stat input path '%s' : %s", input->path, strerror(errno));
		return 1;
	} else if (!stat.exists) {
		BRRLOG_WARN("Cannot parse input '");
		LOG_FORMAT(LOG_PARAMS_PATH, "%s", input->path);
		BRRLOG_WAR("' : Path does not exist");
		return 1;
	} else if (stat.type != brrpath_type_file) {
		BRRLOG_WARN("Cannot parse input '");
		LOG_FORMAT(LOG_PARAMS_PATH, "%s", input->path);
		BRRLOG_WAR("' : Path is not a regular file");
		return 1;
	}
	return 0;
}
static inline int
i_determine_input_type(neinput_t *const input)
{
	FILE *fp = NULL;
	long ext = 0;
	if (-1 != (ext = lib_cmp_ext(input->path, input->path_length, 0, "ogg", "wem", "wsp", "bnk", NULL))) {
		switch (ext) {
			case 0: input->type = neinput_type_ogg; break;
			case 1: input->type = neinput_type_wem; break;
			case 2: input->type = neinput_type_wsp; break;
			case 3: input->type = neinput_type_bnk; break;
		}
		return 0;
	}
	if (!(fp = fopen(input->path, "rb"))) {
		return I_IO_ERROR;
	} else if (4 != fread(&ext, 1, 4, fp)) {
		fclose(fp);
		if (ferror(fp))
			return I_IO_ERROR;
		else
			return I_INSUFFICIENT_DATA;
	}
	fclose(fp);
	if ((brru4)ext == FCC_GET_INT("OggS")) {
		input->type = neinput_type_ogg;
	} else if ((brru4)ext == FCC_GET_INT("RIFF")) {
		input->type = neinput_type_wem;
	} else if ((brru4)ext == FCC_GET_INT("BKHD")) {
		input->type = neinput_type_bnk;
	} else {
		return I_UNRECOGNIZED_DATA;
	}
	return 0;
}

static inline int
i_process_input(nestate_t *const state, neinput_t *const input, brrsz idx)
{
	int err = 0;
	BRRLOG_NORN("Processing input ");
	LOG_FORMAT(LOG_PARAMS_INFO, "%*zu / %zu", state->stats.n_input_digits, idx + 1, state->n_inputs);
	BRRLOG_NORN(" ");
	switch (input->type) {
		case neinput_type_auto: LOG_FORMAT(LOG_PARAMS_AUT, "%-*s", state->stats.input_path_max, input->path, "");
		    err = i_determine_input_type(input); break;
		case neinput_type_ogg: LOG_FORMAT(LOG_PARAMS_OGG, "%-*s", state->stats.input_path_max, input->path, ""); break;
		case neinput_type_wem: LOG_FORMAT(LOG_PARAMS_WEM, "%-*s", state->stats.input_path_max, input->path, ""); break;
		case neinput_type_wsp: LOG_FORMAT(LOG_PARAMS_WSP, "%-*s", state->stats.input_path_max, input->path, ""); break;
		case neinput_type_bnk: LOG_FORMAT(LOG_PARAMS_BNK, "%-*s", state->stats.input_path_max, input->path, ""); break;
	}
	BRRLOG_NORN(" ");
	if (err) {
		BRRLOG_ERR("Failed to determine data type : %s", lib_strerr(err));
		return err;
	}
	switch (input->type) {
		case neinput_type_ogg: return neregrain_ogg(state, input);
		case neinput_type_wem: return neconvert_wem(state, input);
		case neinput_type_wsp: return neextract_wsp(state, input);
		case neinput_type_bnk: return neextract_bnk(state, input);
		default: return I_UNRECOGNIZED_DATA;
	}
}

int
neprocess_inputs(nestate_t *const state)
{
	for (brrsz i = 0; i < state->n_inputs; ++i) {
		neinput_t *const input = &state->inputs[i];
		i_set_log_state(state, input);
		if (i_check_input(input))
			continue;
		BRRLOG_DEBUGN("%sLIST : ", input->filter.type?"BLACK":"WHITE");
		for (brru4 i = 0; i < input->filter.count; ++i) {
			BRRLOG_DEBUGNP("%zu ", input->filter.list[i]);
		}
		BRRLOG_DEBUGP("");
		i_process_input(state, input, i);
	}
	return 0;
}
