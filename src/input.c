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
	} else if (state->next_is_list ||
		       state->next_is_library ||
		       state->next_is_file ||
		       state->always_file) {
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

	else CHECK_TOGGLE_ARG(1, current->inplace_regrain, "-ri", "-rgrn-inplace", "-rvb-inplace")

	else CHECK_TOGGLE_ARG(1, current->inplace_ogg, "-oi", "-ogg-inplace")
	else CHECK_SET_ARG(1, state->next_is_library, 1, "-cbl", "-codebook-library")
	else CHECK_SET_ARG(1, current->library_index, -1, "-inline")
	else CHECK_TOGGLE_ARG(1, current->stripped_headers, "-stripped")

	else CHECK_TOGGLE_ARG(1, current->auto_ogg, "-w2o", "-wem2ogg")
	else IF_CHECK_ARG(1, "-white", "-weiss") {
		current->list.type = 0;
		state->next_is_list = 1;
		return 1;
	}
	else IF_CHECK_ARG(1, "-black", "-noir") {
		current->list.type = 1;
		state->next_is_list = 1;
		return 1;
	}
	else CHECK_TOGGLE_ARG(1, current->list.type, "-rubrum")

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
i_parse_index(char *const arg, int arglen, neinput_listT *const list, int *const offset)
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
				if (!neinput_list_contains(list, val)) {
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
i_set_list(const char *const arg, int arglen, neinput_listT *const list)
{
	int offset = 0;
	while (offset < arglen) {
		if (i_parse_index((char *)arg, arglen, list, &offset))
			return -1;
	}
	return 0;
}
static int
i_add_library(const char *const arg, int arglen, nestateT *const state, neinput_libraryT **const libraries,
    neinputT *const current)
{
	neinput_libraryT next = {.path = arg, .path_length = arglen};
	for (current->library_index = 0; current->library_index < state->n_libraries; ++current->library_index) {
		if (0 == strcmp(libraries[current->library_index]->path, arg))
			return 0;
	}
	/* Not found, add */
	if (brrlib_alloc((void **)libraries, (state->n_libraries + 1) * sizeof(next), 0))
		return -1;
	/* TODO add option to specify library type directly */
	if (-1 != lib_cmp_ext(arg, arglen, 0, "ocbl", NULL))
		next.old = 1;
	(*libraries)[state->n_libraries++] = next;
	return 0;
}
static int
i_add_input(const char *const arg, int arglen, nestateT *const state, neinputT **const inputs,
    neinputT *const current)
{
	neinputT next = *current;
	next.path = arg;
	next.path_length = arglen;
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

void
neinput_list_clear(neinput_listT *const list)
{
	if (list) {
		if (list->list)
			free(list->list);
		memset(list, 0, sizeof(*list));
	}
}
int
neinput_list_contains(const neinput_listT *const list, brru4 index)
{
	if (!list)
		return 0;
	if (!list->list)
		return 0;
	for (brru4 i = 0; i < list->count; ++i) {
		if (list->list[i] == index)
			return 1;
	}
	return 0;
}

void
neinput_clear(neinputT *const input)
{
	if (input) {
		neinput_list_clear(&input->list);
		memset(input, 0, sizeof(*input));
	}
}
int
neinput_take_inputs(nestateT *const state, neinput_libraryT **const libraries,
    neinputT **const inputs, const neinputT default_input, int argc, char **argv)
{
	neinputT current = default_input;
	neinput_listT working_list = {0};
	for (int i = 0; i < argc; ++i) {
		char *arg = argv[i];
		if (i_parse_argument(arg, state, &current)) {
			continue;
		} else if (state->next_is_list) {
			working_list.type = current.list.type;
			if (i_set_list(arg, strlen(arg), &working_list)) {
				/* This probably doesn't need to be a fatal error */
				neinput_clear_all(state, libraries, inputs);
				return -1;
			}
			current.list = working_list;
			state->next_is_list = 0;
		} else if (state->next_is_library) {
			if (i_add_library(arg, strlen(arg), state, libraries, &current)) {
				neinput_clear_all(state, libraries, inputs);
				return -1;
			}
			state->next_is_library = 0;
		} else {
			if (i_add_input(arg, strlen(arg), state, inputs, &current)) {
				neinput_clear_all(state, libraries, inputs);
				return -1;
			}
			state->next_is_file = 0;
			working_list = (neinput_listT){0};
			if (state->should_reset)
				current = default_input;
		}
	}
	state->n_input_digits = brrnum_ndigits(state->n_inputs, 0, 10);
	return 0;
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

	if (!(library->load_error = lib_read_entire_file(library->path, (void **)&buffer, &bufsize))) {
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
int
neinput_load_codebooks(neinput_libraryT *const libraries,
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
