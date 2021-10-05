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
#include <brrtools/brrpath.h>

#include "errors.h"
#include "lib.h"
#include "print.h"

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
	/* TODO add option to specify library type directly */
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
static int
i_parse_library_data(neinput_libraryT *const library, const char *const buffer, brrsz buffer_size)
{
	int err = 0;
	if (library->old) {
		err = codebook_library_deserialize_old(&library->library, buffer, buffer_size);
	} else {
		err = codebook_library_deserialize(&library->library, buffer, buffer_size);
	}
	/* Try again with alternate method */
	if (err == CODEBOOK_CORRUPT) {
		codebook_library_clear(&library->library);
		library->old = !library->old;
		if (library->old) {
			err = codebook_library_deserialize_old(&library->library, buffer, buffer_size);
		} else {
			err = codebook_library_deserialize(&library->library, buffer, buffer_size);
		}
	}
	if (err) {
		codebook_library_clear(&library->library);
		switch (err) {
			case CODEBOOK_CORRUPT: return I_UNRECOGNIZED_DATA;
			case CODEBOOK_ERROR: return I_BUFFER_ERROR;
		}
	}
	return I_SUCCESS;
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

int
neinput_library_load(neinput_libraryT *const library)
{
	char *buffer = NULL;
	brrsz bufsize = 0;
	if (!library)
		return -1;
	else if (library->loaded)
		return 0;
	else if (library->load_error)
		return library->load_error;

	if (!(library->load_error = lib_read_entire_file(library->library_path, (void **)&buffer, &bufsize))) {
		library->load_error = i_parse_library_data(library, buffer, bufsize);
		free(buffer);
	}
	library->loaded = !library->load_error;
	return library->load_error;
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

int
neinput_load_index(neinput_libraryT *const libraries,
    const codebook_libraryT **const library, brrsz index)
{
	if (!library)
		return I_GENERIC_ERROR;
	*library = NULL;
	if (libraries && index != -1) {
		int err = 0;
		neinput_libraryT *inlib = &libraries[index];
		if ((err = neinput_library_load(inlib))) {
			return err;
		}
		*library = &inlib->library;
	}
	return I_SUCCESS;
}
