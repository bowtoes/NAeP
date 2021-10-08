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

#include "process.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <brrtools/brrlog.h>
#include <brrtools/brrpath.h>

#include "lib.h"
#include "errors.h"
#include "print.h"

static void
i_set_log_state(const nestateT *const state, const neinputT *const input)
{
	if (state->log_style_enabled)
		gbrrlogctl.style_enabled = input->log_color_enabled;
#if defined(NeDEBUG)
	gbrrlogctl.debug_enabled = 1;
	brrlog_setmaxpriority(brrlog_priority_debug);
#else
	gbrrlogctl.debug_enabled = input->log_debug;
	brrlog_setmaxpriority(input->log_priority);
#endif
}
static int
i_check_input(const neinputT *const input)
{
	brrpath_stat_resultT stat;
	if (brrpath_stat(&stat, input->path)) {
		BRRLOG_ERR("Failed to stat input path '%s' : %s", input->path, strerror(errno));
		return 1;
	} else if (!stat.exists) {
		BRRLOG_WARN("Cannot parse input '");
		LOG_FORMAT(LOG_PARAMS_PATH, "%s", input->path);
		BRRLOG_WARN("' : Path does not exist");
		return 1;
	} else if (stat.type != brrpath_type_file) {
		BRRLOG_WARN("Cannot parse input '");
		LOG_FORMAT(LOG_PARAMS_PATH, "%s", input->path);
		BRRLOG_WARN("' : Path is not a regular file");
		return 1;
	}
	return 0;
}
static int
i_determine_input_type(neinputT *const input)
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

static int
i_process_input(nestateT *const state, neinput_libraryT *const libraries, neinputT *const input, brrsz idx)
{
	int err = 0;
	BRRLOG_NORN("Processing input ");
	LOG_FORMAT(LOG_PARAMS_INFO, "%*zu / %zu", state->n_input_digits, idx + 1, state->n_inputs);
	BRRLOG_NORN(" ");
	switch (input->type) {
		case neinput_type_auto: LOG_FORMAT(LOG_PARAMS_AUT, "%-*s", state->input_path_max, input->path, "");
		    err = i_determine_input_type(input); break;
		case neinput_type_ogg: LOG_FORMAT(LOG_PARAMS_OGG, "%-*s", state->input_path_max, input->path, ""); break;
		case neinput_type_wem: LOG_FORMAT(LOG_PARAMS_WEM, "%-*s", state->input_path_max, input->path, ""); break;
		case neinput_type_wsp: LOG_FORMAT(LOG_PARAMS_WSP, "%-*s", state->input_path_max, input->path, ""); break;
		case neinput_type_bnk: LOG_FORMAT(LOG_PARAMS_BNK, "%-*s", state->input_path_max, input->path, ""); break;
	}
	BRRLOG_NORN(" ");
	if (err) {
		BRRLOG_ERR("Failed to determine data type : %s", lib_strerr(err));
		return err;
	}
	switch (input->type) {
		case neinput_type_ogg: return neregrain_ogg(state, input);
		case neinput_type_wem: return neconvert_wem(state, libraries, input);
		case neinput_type_wsp: return neextract_wsp(state, libraries, input);
		case neinput_type_bnk: return neextract_bnk(state, libraries, input);
	}
	return I_UNRECOGNIZED_DATA;
}

int
neprocess_inputs(nestateT *const state, neinput_libraryT *const libraries, neinputT *const inputs)
{
	for (brrsz i = 0; i < state->n_inputs; ++i) {
		neinputT *const input = &inputs[i];
		i_set_log_state(state, input);
		if (i_check_input(input))
			continue;
		BRRLOG_DEBUGN("%sLIST : ", input->list.type?"BLACK":"WHITE");
		for (brru4 i = 0; i < input->list.count; ++i) {
			BRRLOG_DEBUGNP("%zu ", input->list.list[i]);
		}
		BRRLOG_DEBUGP("");
		i_process_input(state, libraries, input, i);
	}
	return 0;
}
