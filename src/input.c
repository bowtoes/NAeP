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
#include "input.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(_WIN32)
#include <strings.h>
#endif

#include <brrtools/brrlib.h>
#include <brrtools/brrnum.h>
#include <brrtools/brrstringr.h>
#include <brrtools/brrpath.h>

#include "errors.h"
#include "lib.h"
#include "print.h"

static int
i_mod_priority(neinput_t *const input, int delta)
{
	neinput_t i = *input;
	if (delta < 0) {
		if (i.log_priority > 0)
			i.log_priority--;
	} else if (delta > 0) {
		if (i.log_priority < brrlog_priority_count - 1)
			i.log_priority++;
	}
	i.flag.log_enabled = i.log_priority != 0 || i.flag.log_debug;
	*input = i;
	return 1;
}
// Retruns 0 when an argument is not parsed, 1 when it is.
static int
i_parse_argument(const char *const arg, nestate_t *const state, neinput_t *const current)
{
	if (arg[0] == 0) { /* Argument is of 0 length, very bad! */
		return 1;
	} else if (state->settings.next_is_filter ||
	           state->settings.next_is_library ||
	           state->settings.next_is_file ||
	           state->settings.always_file) {
		return 0;
	}
#define IF_CHECK_ARG(_cse_, ...) if (brrstringr_cstr_compare(arg, _cse_, __VA_ARGS__, NULL))
#define CHECK_TOGGLE_ARG(_c_, _a_, ...) IF_CHECK_ARG((_c_), __VA_ARGS__) { (_a_) = !(_a_); return 1; }
#define CHECK_SET_ARG(_c_, _a_, _v_, ...) IF_CHECK_ARG((_c_), __VA_ARGS__) { (_a_) = (_v_); return 1; }
#define CHECK_RUN_ARG(_c_, _a_, ...) IF_CHECK_ARG((_c_), __VA_ARGS__) { _a_; }
	else CHECK_RUN_ARG(0, return print_help(), "-h", "-help", "--help", "-v", "-version", "--version")
	else CHECK_SET_ARG(1, current->type, neinput_type_auto, "-a", "-auto", "-detect")
	else CHECK_SET_ARG(1, current->type, neinput_type_wem, "-w", "-wem", "-weem")
	else CHECK_SET_ARG(1, current->type, neinput_type_wsp, "-W", "-wsp", "-wisp")
	else CHECK_SET_ARG(1, current->type, neinput_type_bnk, "-b", "-bnk", "-bank")
	else CHECK_SET_ARG(1, current->type, neinput_type_ogg, "-o", "-ogg")

	else CHECK_TOGGLE_ARG(1, current->flag.inplace_regrain, "-ri", "-rgrn-inplace", "-rvb-inplace")

	else CHECK_TOGGLE_ARG(1, current->flag.inplace_ogg, "-oi", "-ogg-inplace")
	else CHECK_SET_ARG(1, state->settings.next_is_library, 1, "-cbl", "-codebook-library")
	else CHECK_SET_ARG(1, current->library_index, -1, "-inline")
	else CHECK_TOGGLE_ARG(1, current->flag.stripped_headers, "-stripped")

	else CHECK_TOGGLE_ARG(1, current->flag.auto_ogg, "-w2o", "-wem2ogg")
	else IF_CHECK_ARG(1, "-white", "-weiss") {
		current->filter.type = neinput_filter_white;
		state->settings.next_is_filter = 1;
		return 1;
	}
	else IF_CHECK_ARG(1, "-black", "-noir") {
		current->filter.type = neinput_filter_black;
		state->settings.next_is_filter = 1;
		return 1;
	}
	else CHECK_TOGGLE_ARG(1, current->filter.type, "-rubrum")

	else CHECK_TOGGLE_ARG(1, state->settings.next_is_file, "-!")
	else CHECK_TOGGLE_ARG(1, state->settings.always_file, "--")
	else CHECK_TOGGLE_ARG(1, current->flag.log_debug, "-d", "-debug")
	else CHECK_TOGGLE_ARG(1, current->flag.log_color_enabled, "-c", "-color")
	else CHECK_TOGGLE_ARG(1, state->settings.log_style_enabled, "-C", "-state-color")
	else CHECK_TOGGLE_ARG(1, state->settings.report_card, "-r", "-report-card")
	else CHECK_TOGGLE_ARG(1, state->settings.full_report, "+r", "-full-report")
	else CHECK_RUN_ARG(1, return i_mod_priority(current, -1), "-q", "-quiet")
	else CHECK_RUN_ARG(1, return i_mod_priority(current, +1), "+q", "+quiet")
	else CHECK_TOGGLE_ARG(1, current->flag.log_enabled, "-Q", "-qq", "too-quiet")
	else CHECK_TOGGLE_ARG(1, current->flag.dry_run, "-n", "-dry", "-dry-run")
	else CHECK_TOGGLE_ARG(1, state->settings.should_reset, "-reset")
#undef IF_CHECK_ARG
#undef CHECK_TOGGLE_ARG
#undef CHECK_SET_ARG
#undef CHECK_RUN_ARG
	else return 0;
}
static int
i_parse_index(char *const arg, int arglen, neinput_filter_t *const list, int *const offset)
{
	int comma = *offset;
	for (;comma < arglen && arg[comma] != ','; ++comma);
	if (comma > *offset) {
		int digit = *offset;
		for (;digit < comma && !isdigit(arg[digit]); ++digit);
		if (digit < comma) {
			char *start = arg + digit, *error = NULL;
			unsigned long long val = 0;
			arg[comma] = 0;
			val = strtoull(start, &error, 0);
			arg[comma] = ',';
			if (start[0] && error[0] == ',') { /*  Complete success */
				if (!neinput_filter_contains(list, val)) {
					if (brrlib_alloc((void **)&list->list, (list->count + 1) * sizeof(*list->list), 0))
						return -1;
					list->list[list->count++] = val;
				}
			}
		}
	}
	*offset = comma + 1;
	return 0;
}
static int
i_set_filter(const char *const arg, int arglen, neinput_filter_t *const filter)
{
	int offset = 0;
	while (offset < arglen) {
		if (i_parse_index((char *)arg, arglen, filter, &offset))
			return -1;
	}
	return 0;
}

void
neinput_filter_clear(neinput_filter_t *const filter)
{
	if (filter) {
		if (filter->list)
			free(filter->list);
		memset(filter, 0, sizeof(*filter));
	}
}
int
neinput_filter_contains(const neinput_filter_t *const filter, brru4 index)
{
	if (!filter)
		return 0;
	if (!filter->list)
		return 0;
	for (brru4 i = 0; i < filter->count; ++i) {
		if (filter->list[i] == index)
			return 1;
	}
	return 0;
}

void
neinput_clear(neinput_t *const input)
{
	if (input) {
		neinput_filter_clear(&input->filter);
		memset(input, 0, sizeof(*input));
	}
}
static int
i_parse_library_data(neinput_library_t *const library, const char *const buffer, brrsz buffer_size)
{
	int err = 0;
	if (library->status.old) {
		err = codebook_library_deserialize_old(&library->library, buffer, buffer_size);
	} else {
		err = codebook_library_deserialize(&library->library, buffer, buffer_size);
	}

	if (err == CODEBOOK_CORRUPT) {
		/* Try again with alternate method */
		codebook_library_clear(&library->library);
		library->status.old = !library->status.old;
		if (library->status.old) {
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
neinput_library_load(neinput_library_t *const library)
{
	if (!library)
		return -1;
	else if (library->status.loaded)
		return 0;
	else if (library->status.load_error)
		return library->status.load_error;

	void *buffer = NULL;
	brrsz bufsize = 0;
	if (!(library->status.load_error = lib_read_entire_file(library->path, &buffer, &bufsize))) {
		library->status.load_error = i_parse_library_data(library, buffer, bufsize);
		free(buffer);
	}
	library->status.loaded = !library->status.load_error;
	return library->status.load_error;
}
void
neinput_library_clear(neinput_library_t *const library)
{
	if (library) {
		codebook_library_clear(&library->library);
		memset(library, 0, sizeof(*library));
	}
}
int
neinput_load_codebooks(neinput_library_t *const libraries, const codebook_library_t **const library, brrsz index)
{
	if (!library)
		return I_GENERIC_ERROR;
	*library = NULL;
	if (libraries && index != -1) {
		int err = 0;
		neinput_library_t *inlib = &libraries[index];
		if ((err = neinput_library_load(inlib))) {
			return err;
		}
		*library = &inlib->library;
	}
	return I_SUCCESS;
}

static int
i_add_library(nestate_t *const state, neinput_t *const current, const char *const arg, int arglen)
{
	neinput_library_t next = {.path = arg, .path_length = arglen};
	for (current->library_index = 0; current->library_index < state->n_libraries; ++current->library_index) {
		if (0 == strcmp(state->libraries[current->library_index].path, arg))
			return 0;
	}
	/* Not found, add */
	if (brrlib_alloc((void **)&state->libraries, (state->n_libraries + 1) * sizeof(next), 0))
		return -1;
	/* TODO add option to specify library type directly */
	if (-1 != lib_cmp_ext(arg, arglen, 0, "ocbl", NULL))
		next.status.old = 1;
	state->libraries[state->n_libraries++] = next;
	return 0;
}
static int
i_add_input(nestate_t *const state, neinput_t *const current, const char *const arg, int arglen)
{
	neinput_t next = *current;
	next.path = arg;
	next.path_length = arglen;
	for (brrsz i = 0; i < state->n_inputs; ++i) {
		if (0 == strcmp(state->inputs[i].path, arg)) {
			state->inputs[i] = next;
			return 0;
		}
	}
	/* Not found, add */
	if (brrlib_alloc((void **)&state->inputs, (state->n_inputs + 1) * sizeof(next), 0))
		return -1;
	state->inputs[state->n_inputs++] = next;
	{
		int n = strlen(next.path);
		if (n > state->stats.input_path_max)
			state->stats.input_path_max = n;
	}
	return 0;
}
int
nestate_init(nestate_t *const state, int argc, char **argv)
{
	neinput_t current = state->default_input;
	neinput_filter_t working_filter = {0};
	for (int i = 0; i < argc; ++i) {
		char *arg = argv[i];
		if (i_parse_argument(arg, state, &current)) {
			continue;
		} else if (state->settings.next_is_filter) {
			working_filter.type = current.filter.type;
			if (i_set_filter(arg, strlen(arg), &working_filter)) {
				/* This probably doesn't need to be a fatal error */
				nestate_clear(state);
				return -1;
			}
			current.filter = working_filter;
			state->settings.next_is_filter = 0;
		} else if (state->settings.next_is_library) {
			if (i_add_library(state, &current, arg, strlen(arg))) {
				nestate_clear(state);
				return -1;
			}
			state->settings.next_is_library = 0;
		} else {
			if (i_add_input(state, &current, arg, strlen(arg))) {
				nestate_clear(state);
				return -1;
			}
			state->settings.next_is_file = 0;
			working_filter = (neinput_filter_t){0};
			if (state->settings.should_reset)
				nestate_clear(state);
		}
	}
	state->stats.n_input_digits = brrnum_ndigits(state->n_inputs, 0, 10);
	return 0;
}

void
nestate_clear(nestate_t *const state)
{
	if (!state)
		return;
	if (state->inputs) {
		for (brrsz i = 0; i < state->n_inputs; ++i)
			neinput_clear(&(state->inputs)[i]);
		free(state->inputs);
	}
	if (state->libraries) {
		for (brrsz i = 0; i < state->n_libraries; ++i)
			neinput_library_clear(&(state->libraries)[i]);
		free(state->libraries);
	}
	memset(state, 0, sizeof(*state) - sizeof(state->stats));
}
