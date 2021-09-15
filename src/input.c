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

#include "input.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(_WIN32)
#include <strings.h>
#endif

#include <brrtools/brrlib.h>
#include <brrtools/brrstg.h>

#include "print.h"

#if 0
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <brrtools/brrlib.h>
#include <brrtools/brrlog.h>
#include <brrtools/brrpath.h>

#include "common_lib.h"
#include "errors.h"

static int BRRCALL
i_try_load_lib(input_libraryT *const library, const unsigned char *const buffer, brrsz buff_size)
{
	int err = 0;
	if (library->old)
		err = codebook_library_deserialize_deprecated(&library->library, buffer, buff_size);
	else
		err = codebook_library_deserialize(&library->library, buffer, buff_size);
	if (err == CODEBOOK_CORRUPT) {
		codebook_library_clear(&library->library);
		library->old = !library->old;
		if (library->old)
			err = codebook_library_deserialize_deprecated(&library->library, buffer, buff_size);
		else
			err = codebook_library_deserialize(&library->library, buffer, buff_size);
	}
	if (err == CODEBOOK_CORRUPT) {
		codebook_library_clear(&library->library);
		err = I_UNRECOGNIZED_DATA;
	} else if (err == CODEBOOK_ERROR) {
		err = I_BUFFER_ERROR;
	}
	return err;
}
static int BRRCALL
i_try_read_lib(input_libraryT *const library, unsigned char **const buffer, brrsz *file_size)
{
	brrpath_stat_resultT rs;
	int err = 0;
	FILE *file = NULL;
	if ((err = brrpath_stat(&rs, library->library_path.opaque))) {
		return I_IO_ERROR;
	} else if (!rs.exists || rs.type != brrpath_type_file) {
		return I_IO_ERROR;
	} else if ((brrlib_alloc((void **)buffer, rs.size, 1))) {
		return I_BUFFER_ERROR;
	} else if (!(file = fopen(library->library_path.opaque, "rb"))) {
		free(*buffer);
		*buffer = NULL;
		return I_IO_ERROR;
	} else if (rs.size > fread(*buffer, 1, rs.size, file)) {
		err = feof(file)?I_FILE_TRUNCATED:I_IO_ERROR;
		free(*buffer);
		*buffer = NULL;
	}
	fclose(file);
	if (!err)
		*file_size = rs.size;
	return err;
}
int BRRCALL
input_library_load(input_libraryT *const library)
{
	unsigned char *buffer = NULL;
	brrsz bufsize = 0;
	if (library->loaded) {
		return 0;
	} else if (library->load_error) {
		return library->load_error;
	} else if (!(library->load_error = i_try_read_lib(library, &buffer, &bufsize))) {
		library->load_error = i_try_load_lib(library, buffer, bufsize);
		free(buffer);
	}
	library->loaded = !library->load_error;
	return library->load_error;
}
void BRRCALL
input_library_clear(input_libraryT *const library)
{
	if (library) {
		codebook_library_clear(&library->library);
		brrstg_delete(&library->library_path);
		memset(library, 0, sizeof(*library));
	}
}

void BRRCALL
input_clear(inputT *const input)
{
	if (input) {
		brrstg_delete(&input->path);
		memset(input, 0, sizeof(*input));
	}
}
void BRRCALL
input_print(const inputT *const input, brrsz max_input_length,
    brrlog_priorityT priority, int newline)
{
	gbrrlog_level_last = gbrrlog_level_debug;
	gbrrlog_format_last = gbrrlog_format_normal;
	BRRLOG_LASTNP(" ");
	if (input->options.type == input_type_ogg)      { BRRLOG_MESSAGETNP(gbrrlog_level_last, LOG_FORMAT_OGG, "OGG"); }
	else if (input->options.type == input_type_wem) { BRRLOG_MESSAGETNP(gbrrlog_level_last, LOG_FORMAT_WEM, "WEM"); }
	else if (input->options.type == input_type_wsp) { BRRLOG_MESSAGETNP(gbrrlog_level_last, LOG_FORMAT_WSP, "WSP"); }
	else if (input->options.type == input_type_bnk) { BRRLOG_MESSAGETNP(gbrrlog_level_last, LOG_FORMAT_BNK, "BNK"); }
	else                                            { BRRLOG_MESSAGETNP(gbrrlog_level_last, LOG_FORMAT_AUT, "AUT"); }

	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " cbl ");
	BRRLOG_STYLENP(LOG_COLOR_ENABLED, -1, brrlog_style_bold, "%zu", input->options.library_index);
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " log ");
	if (input->options.log_enabled) { BRRLOG_STYLENP(LOG_COLOR_ENABLED, -1, brrlog_style_bold, "ENB"); }
	else { BRRLOG_FORENP(LOG_COLOR_DISABLED, "DSB"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " ");
	if (input->options.log_color_enabled) { BRRLOG_STYLENP(LOG_COLOR_ENABLED, -1, brrlog_style_bold, "STY"); }
	else { BRRLOG_FORENP(LOG_COLOR_DISABLED, "SMP"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " ");
	if (input->options.log_debug) { BRRLOG_FORENP(brrlog_color_cyan, "DBG"); }
	else { BRRLOG_FORENP(brrlog_color_yellow, "NRM"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " ");
	BRRLOG_FORENP(brrlog_color_normal + 1 + input->options.log_priority,
	    "%s", brrlog_priority_dbgstr(input->options.log_priority));
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " operation ");
	if (input->options.dry_run) { BRRLOG_FORENP(LOG_COLOR_DRY, "DRY"); }
	else { BRRLOG_FORENP(LOG_COLOR_WET, "WET"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " w2o ");
	if (input->options.auto_ogg) { BRRLOG_FORENP(LOG_COLOR_AUT, "AUT"); }
	else { BRRLOG_FORENP(LOG_COLOR_MANUAL, "MAN"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " ");
	if (input->options.inplace_ogg) { BRRLOG_FORENP(LOG_COLOR_INPLACE, "INP"); }
	else { BRRLOG_FORENP(LOG_COLOR_SEPARATE, "SEP"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " rvb ");
	if (input->options.auto_ogg) { BRRLOG_FORENP(LOG_COLOR_AUT, "AUT"); }
	else { BRRLOG_FORENP(LOG_COLOR_MANUAL, "MAN"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " ");
	if (input->options.inplace_ogg) { BRRLOG_FORENP(LOG_COLOR_INPLACE, "INP"); }
	else { BRRLOG_FORENP(LOG_COLOR_MANUAL, "SEP"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " bank recurse ");
	if (input->options.bank_recurse) { BRRLOG_FORENP(LOG_COLOR_ENABLED, "FUL"); }
	else { BRRLOG_FORENP(LOG_COLOR_DISABLED, "NON"); }
	if (newline)
		BRRLOG_LASTP("");
}
#endif

/* -1 : Not found */
static int
i_find_ext(const char *const arg)
{
	int st = strlen(arg);
	int dot = st;
	for (int i = st; i > 0; --i) {
		char c = arg[i - 1];
		if ('.' == c) {
			dot = i - 1;
		} else
#if defined(_WIN32)
		if ('/' == c || '\\' == c)
#else
		if ('/' == c)
#endif
		{
			break;
		}
	}
	if (dot == st) /* No dot found */
		return -1;
	return dot;
}

static int
i_mod_priority(neinputT *const input, int delta)
{
	if (delta <= 0) {
		if (input->log_priority > 0)
			input->log_priority--;
		if (input->log_priority == 0 && !input->log_debug)
			input->log_enabled = 0;
	} else {
		if (input->log_priority < brrlog_priority_count - 1)
			input->log_priority++;
		input->log_enabled = 1;
	}
	return 1;
}
static int
i_parse_argument(const char *const arg, nestateT *const state, neinputT *const current)
{
	if (arg[0] == 0) { /* Argument is of 0 length, very bad! */
		return 1;
	} else if (state->always_file) {
		return 0;
	} else if (state->next_is_file) {
		state->next_is_file = 0;
		return 0;
	} else if (state->next_is_library) {
		return 0;
	}
#define IF_CHECK_ARG(_cse_, ...) if (-1 != brrstg_cstr_compare(arg, _cse_, __VA_ARGS__, NULL))
#define CHECK_TOGGLE_ARG(_c_, _a_, ...) IF_CHECK_ARG((_c_), __VA_ARGS__) { (_a_) = !(_a_); return 1; }
#define CHECK_SET_ARG(_c_, _a_, _v_, ...) IF_CHECK_ARG((_c_), __VA_ARGS__) { (_a_) = (_v_); return 1; }
#define CHECK_RUN_ARG(_c_, _a_, ...) IF_CHECK_ARG((_c_), __VA_ARGS__) { _a_; }
	else CHECK_RUN_ARG(0, return print_help(), "-h", "-help", "--help", "-v", "-version", "--version")
	else CHECK_SET_ARG(1, current->type, neinput_type_auto, "-a", "-auto", "-detect")
	else CHECK_SET_ARG(1, current->type, neinput_type_wem, "-w", "-wem", "-weem")
	else CHECK_SET_ARG(1, current->type, neinput_type_wsp, "-W", "-wsp", "-wisp")
	else CHECK_SET_ARG(1, current->type, neinput_type_bnk, "-b", "-bnk", "-bank")
	else CHECK_SET_ARG(1, current->type, neinput_type_ogg, "-o", "-ogg")

	else CHECK_TOGGLE_ARG(1, current->inplace_regrain, "-ri", "-rgrn-inplace", "-rvb-inplace")

	else CHECK_TOGGLE_ARG(1, current->inplace_ogg, "-oi", "-ogg-inplace")
	else CHECK_SET_ARG(1, state->next_is_library, 1, "-cbl", "-codebook-library")
	else CHECK_SET_ARG(1, current->library_index, -1, "-inline")
	else CHECK_TOGGLE_ARG(1, current->stripped_headers, "-stripped")

	else CHECK_TOGGLE_ARG(1, current->auto_ogg, "-w2o", "-wem2ogg")
	else CHECK_TOGGLE_ARG(1, current->bank_recurse, "-Rbnk", "-recurse_bank")

	else CHECK_TOGGLE_ARG(1, state->next_is_file, "-!")
	else CHECK_TOGGLE_ARG(1, state->always_file, "--")
	else CHECK_TOGGLE_ARG(1, current->log_debug, "-d", "-debug")
	else CHECK_TOGGLE_ARG(1, current->log_color_enabled, "-c", "-color")
	else CHECK_TOGGLE_ARG(1, state->log_style_enabled, "-C", "-state-color")
	else CHECK_TOGGLE_ARG(1, state->report_card, "-r", "-report-card")
	else CHECK_TOGGLE_ARG(1, state->full_report, "+r", "-full-report")
	else CHECK_RUN_ARG(1, return i_mod_priority(current, -1), "-q", "-quiet")
	else CHECK_RUN_ARG(1, return i_mod_priority(current, +1), "+q", "+quiet")
	else CHECK_TOGGLE_ARG(1, current->log_enabled, "-Q", "-qq", "too-quiet")
	else CHECK_TOGGLE_ARG(1, current->dry_run, "-n", "-dry", "-dry-run")
	else CHECK_TOGGLE_ARG(1, state->should_reset, "-reset")
#undef IF_CHECK_ARG
#undef CHECK_TOGGLE_ARG
#undef CHECK_SET_ARG
#undef CHECK_RUN_ARG
	else return 0;
}
static int
i_add_library(const char *const arg, nestateT *const state, neinput_libraryT **const libraries,
    neinputT *const current)
{
	neinput_libraryT next = {.library_path = arg};
	for (current->library_index = 0; current->library_index < state->n_libraries; ++current->library_index) {
		if (0 == strcmp(libraries[current->library_index]->library_path, arg))
			return 0;
	}
	/* Not found, add */
	if (brrlib_alloc((void **)libraries, (state->n_libraries + 1) * sizeof(next), 0))
		return -1;
	if (-1 != neinput_check_extension(arg, 0, "ocbl", NULL))
		next.old = 1;
	(*libraries)[state->n_libraries++] = next;
	return 0;
}
static int
i_add_input(const char *const arg, nestateT *const state, neinputT **const inputs,
    neinputT *const current)
{
	neinputT next = *current;
	next.path = arg;
	for (brrsz i = 0; i < state->n_inputs; ++i) {
		if (0 == strcmp((*inputs)[i].path, arg)) {
			(*inputs)[i] = next;
			return 0;
		}
	}
	/* Not found, add */
	if (brrlib_alloc((void **)inputs, (state->n_inputs + 1) * sizeof(next), 0))
		return -1;
	(*inputs)[state->n_inputs++] = next;
	{
		int n = strlen(next.path);
		if (n > state->input_path_max)
			state->input_path_max = n;
	}
	return 0;
}

int
neinput_check_extension(const char *const arg, int case_sensitive, ...)
{
	static char ext[513] = {0};
	const char *a;
	va_list lptr;
	int (*cmp)(const char *const, const char *const);
	int suc = 0, idx = 0;

	if (-1 == (idx = i_find_ext(arg)))
		return -1;
	snprintf(ext, 513, "%s", arg + idx);

	if (case_sensitive) {
		cmp = strcmp;
	} else {
#if defined(_WIN32)
		cmp = _stricmp;
#else
		cmp = strcasecmp;
#endif
	}
	va_start(lptr, case_sensitive);
	while ((a = va_arg(lptr, const char *))) {
		if (0 == cmp(arg, a)) {
			suc = 1;
			break;
		}
		idx++;
	}
	va_end(lptr);
	return suc?idx:-1;
}

int
neinput_take_inputs(nestateT *const state, neinput_libraryT **const libraries,
    neinputT **const inputs, const neinputT default_input, int argc, char **argv)
{
	neinputT current = default_input;
	for (int i = 0; i < argc; ++i) {
		char *arg = argv[i];
		if (i_parse_argument(arg, state, &current)) {
			continue;
		} else if (state->next_is_library) {
			if (i_add_library(arg, state, libraries, &current)) {
				neinput_clear_all(state, libraries, inputs);
				return -1;
			}
			state->next_is_library = 0;
		} else {
			if (i_add_input(arg, state, inputs, &current)) {
				neinput_clear_all(state, libraries, inputs);
				return -1;
			}
		}
		if (state->should_reset)
			current = default_input;
	}
	state->n_input_digits = brrlib_ndigits(state->n_inputs, 0, 10);
	return 0;
}

void
neinput_clear(neinputT *const input)
{
	if (input) {
		memset(input, 0, sizeof(*input));
	}
}

void
neinput_library_clear(neinput_libraryT *const library)
{
	if (library) {
		codebook_library_clear(&library->library);
		memset(library, 0, sizeof(*library));
	}
}

void
neinput_clear_all(const nestateT *const state, neinput_libraryT **const libraries,
    neinputT **const inputs)
{
	if (!state)
		return;
	if (inputs && *inputs) {
		for (brrsz i = 0; i < state->n_inputs; ++i) {
			neinput_clear(&(*inputs)[i]);
		}
		free(*inputs);
		*inputs = NULL;
	}
	if (libraries && *libraries) {
		for (brrsz i = 0; i < state->n_libraries; ++i) {
			neinput_library_clear(&(*libraries)[i]);
		}
		free(*libraries);
		*libraries = NULL;
	}
}
