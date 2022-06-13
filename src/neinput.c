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
#include "neinput.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <brrtools/brrnum.h>

// Retruns 0 when an argument is not parsed, 1 when it is.
static inline int
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

#define _mod_priority(_input_, _delta_) do {\
	if ((_delta_) < 0 && (_input_)->log_priority > 0)\
		(_input_)->log_priority--;\
	else if ((_delta_) > 0 && (_input_)->log_priority < brrlog_priority_count - 1)\
		(_input_)->log_priority++;\
	(_input_)->flag.log_enabled = (_input_)->flag.log_debug || (_input_)->log_priority;\
} while (0)

#define IF_CHECK_ARG(_cse_, ...) if (brrstringr_cstr_compare(arg, _cse_, __VA_ARGS__, NULL))
#define CHECK_TOGGLE_ARG(_c_, _a_, ...) IF_CHECK_ARG((_c_), __VA_ARGS__) { (_a_) = !(_a_); return 1; }
#define CHECK_SET_ARG(_c_, _a_, _v_, ...) IF_CHECK_ARG((_c_), __VA_ARGS__) { (_a_) = (_v_); return 1; }
#define CHECK_RUN_ARG(_c_, _a_, ...) IF_CHECK_ARG((_c_), __VA_ARGS__) { _a_; }
	else CHECK_RUN_ARG(0, return print_help(), "-h", "-help", "--help", "-v", "-version", "--version")
	else CHECK_SET_ARG(1, current->data_type, nedatatype_auto, "-a", "-auto", "-detect")
	else CHECK_SET_ARG(1, current->data_type, nedatatype_wem, "-w", "-wem", "-weem")
	else CHECK_SET_ARG(1, current->data_type, nedatatype_wsp, "-W", "-wsp", "-wisp")
	else CHECK_SET_ARG(1, current->data_type, nedatatype_bnk, "-b", "-bnk", "-bank")
	else CHECK_SET_ARG(1, current->data_type, nedatatype_ogg, "-o", "-ogg")

	else CHECK_TOGGLE_ARG(1, current->flag.inplace_regrain, "-ri", "-rgrn-inplace", "-rvb-inplace")

	else CHECK_TOGGLE_ARG(1, current->flag.inplace_ogg, "-oi", "-ogg-inplace")
	else CHECK_SET_ARG(1, state->settings.next_is_library, 1, "-cbl", "-codebook-library")
	else CHECK_SET_ARG(1, current->library_index, -1, "-inline")
	else CHECK_TOGGLE_ARG(1, current->flag.stripped_headers, "-stripped")

	else CHECK_TOGGLE_ARG(1, current->flag.auto_ogg, "-w2o", "-wem2ogg")
	else IF_CHECK_ARG(1, "-white", "-weiss") {
		current->filter.type = nefilter_white;
		state->settings.next_is_filter = 1;
		return 1;
	}
	else IF_CHECK_ARG(1, "-black", "-noir") {
		current->filter.type = nefilter_black;
		state->settings.next_is_filter = 1;
		return 1;
	}
	else CHECK_TOGGLE_ARG(1, current->filter.type, "-rubrum")

	else CHECK_TOGGLE_ARG(1, state->settings.next_is_file, "-!")
	else CHECK_TOGGLE_ARG(1, state->settings.always_file, "--")
	else CHECK_TOGGLE_ARG(1, current->flag.add_comments, "-co", "-comments")
	else CHECK_TOGGLE_ARG(1, current->flag.log_debug, "-d", "-debug")
	else CHECK_TOGGLE_ARG(1, current->flag.log_color_enabled, "-c", "-color")
	else CHECK_TOGGLE_ARG(1, state->settings.log_style_enabled, "-C", "-state-color")
	else CHECK_TOGGLE_ARG(1, state->settings.report_card, "-r", "-report-card")
	else CHECK_TOGGLE_ARG(1, state->settings.full_report, "+r", "-full-report")
	else CHECK_RUN_ARG(1, _mod_priority(current, -1); return 1, "-q", "-quiet")
	else CHECK_RUN_ARG(1, _mod_priority(current, +1); return 1, "+q", "+quiet")
	else CHECK_TOGGLE_ARG(1, current->flag.log_enabled, "-Q", "-qq", "too-quiet")
	else CHECK_TOGGLE_ARG(1, current->flag.dry_run, "-n", "-dry", "-dry-run")
	else CHECK_TOGGLE_ARG(1, state->settings.should_reset, "-reset")
#undef IF_CHECK_ARG
#undef CHECK_TOGGLE_ARG
#undef CHECK_SET_ARG
#undef CHECK_RUN_ARG
	else return 0;
}

void
neinput_clear(neinput_t *const input)
{
	if (input) {
		nefilter_clear(&input->filter);
		memset(input, 0, sizeof(*input));
	}
}

static inline int
i_parse_library_data(neinput_library_t *const library, const char *const buffer, brrsz buffer_size)
{
	int s = codebook_library_check_type(buffer, buffer_size);
	if (s == -1) {
		Err(,"Codebook library '%s' is corrupt or isn't a codebook library", library->path.cstr);
		return -1;
	}

	library->status.alternate = s;
	int err = 0;
	if (library->status.alternate) {
		err = codebook_library_deserialize_alt(&library->library, buffer, buffer_size);
	} else {
		err = codebook_library_deserialize(&library->library, buffer, buffer_size);
	}

	if (err) {
		codebook_library_clear(&library->library);
		switch (err) {
			case CODEBOOK_ERROR:
				Err(,"Error while processing codebook library '%s'", library->path.cstr); break;
			case CODEBOOK_CORRUPT:
				Err(,"Codebook library '%s' contains corrupt data", library->path.cstr); break;
			default:
				Err(,"Unrecognized error while processing codebook library '%s' : %d", library->path.cstr, err); break;
		}
		return -1;
	}
	return 0;
}

int
neinput_library_load(neinput_library_t *const library)
{
	if (!library)
		return -1;
	if (library->status.load_error)
		return library->status.load_error;
	if (library->status.loaded)
		return 0;

	void *buffer = NULL;
	if (!(library->status.load_error = nepath_read(&library->path, &buffer))) {
		library->status.load_error = i_parse_library_data(library, buffer, library->path.st.size);
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
		return -1;

	*library = NULL;
	if (libraries && index != -1) {
		int err = 0;
		neinput_library_t *inlib = &libraries[index];
		if ((err = neinput_library_load(inlib))) {
			return err;
		}
		*library = &inlib->library;
	}
	return 0;
}

static inline int
i_add_library(nestate_t *const state, neinput_t *const current, const char *const arg)
{
	for (current->library_index = 0; current->library_index < state->n_libraries; ++current->library_index) {
		if (0 == brrpathcmp(state->libraries[current->library_index].path.cstr, arg))
			return 0;
	}

	/* Not found, add */
	neinput_library_t next = {0};
	if (nepath_init(&next.path, arg))
		return -1;

	neinput_library_t *new = realloc(state->libraries, sizeof(*new) * (state->n_libraries + 1));
	if (!new) {
		Err(,"Failed to allocate space for new library '%s' : %s (%d)", arg, strerror(errno), errno);
		return -1;
	}

	/* TODO add option to specify library type directly */
	if (-1 != nepath_extension_cmp(&next.path, NULL, "ocbl", NULL))
		next.status.alternate = 1;

	new[state->n_libraries++] = next;
	state->libraries = new;
	return 0;
}

static inline int
i_add_input(nestate_t *const state, neinput_t *const current, const char *const arg)
{
	neinput_t next = *current;
	if (nepath_init(&next.path, arg)) {
		return -1;
	}
	if (next.path.st.type != brrpath_type_file) {
		Err(,"Input '%s' is not a file", arg);
		return 0;
	}

	for (brrsz i = 0; i < state->n_inputs; ++i) {
		if (0 == brrpathcmp(state->inputs[i].path.cstr, next.path.cstr)) {
			state->inputs[i] = next;
			return 0;
		}
	}

	/* Not found, add */
	neinput_t *new = realloc(state->inputs, sizeof(*new) * (state->n_inputs + 1));
	if (!new) {
		Err(,"Failed to allocate space for next input '%s' : %s (%d)", arg, strerror(errno), errno);
		return -1;
	}
	new[state->n_inputs++] = next;
	state->inputs = new;

	if (next.path.length > state->stats.input_path_max)
		state->stats.input_path_max = next.path.length;
	return 0;
}

int
nestate_init(nestate_t *const state, int argc, char **argv)
{
	neinput_t current = state->default_input;
	nefilter_t working_filter = {0};
	for (int i = 0; i < argc; ++i) {
		const char *arg = argv[i];
		if (i_parse_argument(arg, state, &current)) {
			continue;
		} else if (state->settings.next_is_filter) {
			working_filter.type = current.filter.type;
			if (nefilter_init(&working_filter, arg)) {
				nestate_clear(state);
				return -1;
			}
			current.filter = working_filter;
			state->settings.next_is_filter = 0;
		} else {
			if (state->settings.next_is_library) {
				if (i_add_library(state, &current, arg)) {
					nestate_clear(state);
					return -1;
				}
				state->settings.next_is_library = 0;
			} else {
				if (i_add_input(state, &current, arg)) {
					nestate_clear(state);
					return -1;
				}
				state->settings.next_is_file = 0;
				working_filter = (nefilter_t){0};
				if (state->settings.should_reset)
					current = state->default_input;
			}
		}
	}
	state->stats.n_input_digits = brrnum_ndigits(state->n_inputs, 10, 0);
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
