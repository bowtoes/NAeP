/* Copyright (c), bowtoes (bow.toes@mailfence.com)
Apache 2.0 license, http://www.apache.org/licenses/LICENSE-2.0
Full license can be found in 'license' file */

#include "neinput.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <brrtools/brrstr.h>
#include <brrtools/brrfile.h>
#include <brrtools/brrnum.h>

#include "nelog.h"

static inline void
neinput_copy_settings(neinput_t *const dst, const neinput_t *const src)
{
	if (!dst || !src)
		return;
	/* This works because the path, the last member of neinput, isn't what one wants to copy */
	memcpy(dst, src, sizeof(*dst) - sizeof(dst->path));
}

void
neinput_free(neinput_t *const input)
{
	if (input) {
		brrpath_free(&input->path);
		nefilter_clear(&input->filter);
		memset(input, 0, sizeof(*input));
	}
}

static inline int
i_parse_library_data(neinput_library_t *const library, const char *const buffer, brrsz buffer_size)
{
	int s = codebook_library_check_type(buffer, buffer_size);
	if (s == -1) {
		Err(,"Codebook library '%s' is corrupt or isn't a codebook library", library->path.full);
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
				Err(,"Error while processing codebook library '%s'", library->path.full); break;
			case CODEBOOK_CORRUPT:
				Err(,"Codebook library '%s' contains corrupt data", library->path.full); break;
			default:
				Err(,"Unrecognized error while processing codebook library '%s' : %d", library->path.full, err); break;
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
	brrsz size_read = brrfile_read((const char *)library->path.full, &library->path.inf, &buffer);
	if (size_read == BRRSZ_MAX) {
		Err(,"Libary read failure: %s", brrapi_error_message(nemessage, nemessage_len));
		return -1;
	}

	library->status.load_error = i_parse_library_data(library, buffer, brrpath_get_size(library->path.size));
	free(buffer);
	library->status.loaded = !library->status.load_error;
	return library->status.load_error;
}

void
neinput_library_free(neinput_library_t *const library)
{
	if (library) {
		brrpath_free(&library->path);
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
i_add_library(nestate_t *const state, neinput_t *const current, brrpath_t path)
{
	/* Check if we already have added this path */
	for (current->library_index = 0; current->library_index < state->n_libraries; ++current->library_index) {
		if (brrpath_same(&state->libraries[current->library_index].path, &path))
			return 0;
	}

	/* Duplicate not found, add to list */
	neinput_library_t *new = realloc(state->libraries, sizeof(*new) * (state->n_libraries + 1));
	if (!new) {
		Err(,"Failed to allocate space for new library '%s' : %s (%d)", path.full, strerror(errno), errno);
		return -1;
	}

	neinput_library_t next = {.path =  path};
	/* TODO add option to explicitly specify library type */
	if (-1 != brrpath_match_extension(&path, 1, 0, NULL, "ocbl", NULL))
		next.status.alternate = 1;
	new[state->n_libraries++] = next;
	state->libraries = new;
	return 0;
}

static inline int
i_add_input(nestate_t *const state, neinput_t *const current, brrpath_t path)
{
	/* Search for duplicate inputs */
	for (brrsz i = 0; i < state->n_inputs; ++i) {
		neinput_t *const n = &state->inputs[i];
		if (n->path.len == path.len && 0 == brrpath_cmp(n->path.full, path.full, path.len)) {
			neinput_copy_settings(n, current);
			return 0;
		}
	}

	/* Not found, add */
	neinput_t *new = realloc(state->inputs, sizeof(*new) * (state->n_inputs + 1));
	if (!new) {
		Err(,"Failed to allocate space for next input '%s' : %s (%d)",  path.full, strerror(errno), errno);
		return -1;
	}
	neinput_t next = *current;
	next.path = path;
	new[state->n_inputs++] = next;
	state->inputs = new;

	if (next.path.len > state->stats.input_path_max)
		state->stats.input_path_max = next.path.len;
	return 0;
}

// Returns 0 when an argument is not parsed, 1 when it is.
static inline int
i_parse_setting(const char *const arg, nestate_t *const state, neinput_t *const current)
{
	const brrstr_t varg = brrstr_view_char(arg, -1);
	#define _K(s) #s
	#define K(s) _K(s)
	#define _J(a, b) a ## b
	#define J(a, b) _J(a, b)

	char *A = K(J(_ck_arg, 1));

	if (arg[0] == 0) { /* Argument is of 0 length, very bad! */
		return 1;
	} else if (state->cfg.next_is_filter ||
	           state->cfg.next_is_library ||
	           state->cfg.next_is_file ||
	           state->cfg.always_file) {
		return 0;
	}
	#define _ck_arg(_cse_, ...) ((_cse_) ? brrstr_ncmp(varg, __VA_ARGS__, NULL) : brrstr_incmp(varg, __VA_ARGS__, NULL))

	#define _mod_priority(_input_, _delta_) do {\
		if ((_delta_) < 0 && (_input_)->log_label > logpri_min)\
			(_input_)->log_label--;\
		else if ((_delta_) > 0 && (_input_)->log_label < logpri_max)\
			(_input_)->log_label++;\
		(_input_)->cfg.log_enabled = (_input_)->cfg.log_debug || (_input_)->log_label != logpri_debug;\
	} while (0)

	#define _toggle(_a_) ((_a_) = !(_a_))
	#define IF_CK_ARG(_cse_, ...) if (_ck_arg(_cse_, __VA_ARGS__))
	#define CK_TGL_ARG(_c_, _a_, ...) IF_CK_ARG((_c_), __VA_ARGS__) { _toggle(_a_); return 1; }
	#define CK_SET_ARG(_c_, _a_, _v_, ...) IF_CK_ARG((_c_), __VA_ARGS__) { (_a_) = (_v_); return 1; }
	#define CK_RUN_ARG(_c_, _a_, ...) IF_CK_ARG((_c_), __VA_ARGS__) { _a_; }
	else CK_RUN_ARG(1, _mod_priority(current, +1); return 1, "+q", "+quiet")
	else CK_RUN_ARG(0, return print_help(), "-h", "-help", "--help", "-v", "-version", "--version")
	else CK_RUN_ARG(1, _mod_priority(current, -1); return 1, "-q", "-quiet")
	else CK_SET_ARG(1, current->cfg.data_type, nedatatype_auto, "-a", "-auto", "-detect")
	else CK_SET_ARG(1, current->cfg.data_type, nedatatype_bnk, "-b", "-bnk", "-bank")
	else CK_SET_ARG(1, current->cfg.data_type, nedatatype_ogg, "-o", "-ogg", "-vorbis")
	else CK_SET_ARG(1, current->cfg.data_type, nedatatype_wem, "-w", "-wem", "-weem")
	else CK_SET_ARG(1, current->cfg.data_type, nedatatype_wsp, "-W", "-wsp", "-wisp")
	else CK_SET_ARG(1, current->library_index, -1, "-inline")
	else CK_TGL_ARG(1, current->filter.type, "-rubrum")
	else CK_TGL_ARG(1, current->cfg.add_comments, "-co", "-comments")
	else CK_TGL_ARG(1, current->cfg.auto_ogg, "-w2o", "-wem2ogg")
	else CK_TGL_ARG(1, current->cfg.dry_run, "-n", "-dry", "-dry-run")
	else CK_TGL_ARG(1, current->cfg.inplace_ogg, "-oi", "-ogg-inplace")
	else CK_TGL_ARG(1, current->cfg.inplace_regrain, "-ri", "-rgrn-inplace", "-rvb-inplace")
	else CK_TGL_ARG(1, current->cfg.log_color_enabled, "-c", "-color")
	else CK_TGL_ARG(1, current->cfg.log_debug, "-d", "-debug")
	else CK_TGL_ARG(1, current->cfg.log_enabled, "-Q", "-qq", "too-quiet")
	else CK_TGL_ARG(1, current->cfg.stripped_headers, "-stripped")

	else CK_SET_ARG(1, state->cfg.next_is_library, 1, "-cbl", "-codebook-library")
	else IF_CK_ARG(1, "-white", "-weiss") {
		current->filter.type = nefilter_white;
		state->cfg.next_is_filter = 1;
		return 1;
	}
	else IF_CK_ARG(1, "-black", "-noir") {
		current->filter.type = nefilter_black;
		state->cfg.next_is_filter = 1;
		return 1;
	}
	else CK_TGL_ARG(1, state->cfg.next_is_file, "-!")
	else CK_TGL_ARG(1, state->cfg.always_file, "--")
	else CK_RUN_ARG(1, _toggle(state->cfg.log_style_enabled); brrlog_config.style_enabled = state->cfg.log_style_enabled; return 1, "-C", "-global-color")
	else CK_TGL_ARG(1, state->cfg.report_card, "-r", "-report-card")
	else CK_TGL_ARG(1, state->cfg.full_report, "+r", "-full-report")
	else CK_TGL_ARG(1, state->cfg.should_reset, "-reset")
	#undef IF_CK_ARG
	#undef CK_TGL_ARG
	#undef CK_SET_ARG
	#undef CK_RUN_ARG
	else return 0;
}

int
nestate_init(nestate_t *const state, int argc, char **argv)
{
	neinput_t current = state->default_input;
	nefilter_t working_filter = {0};
	for (int i = 0; i < argc; ++i) {
		const char *arg = argv[i];
		if (i_parse_setting(arg, state, &current)) {
			continue;
		} else if (state->cfg.next_is_filter) {
			working_filter.type = current.filter.type;
			if (nefilter_init(&working_filter, arg)) {
				nestate_clear(state);
				return -1;
			}
			current.filter = working_filter;
			state->cfg.next_is_filter = 0;
		} else {
			brrpath_t path;
			/* Passing an invalid path shouldn't be fatal, correct? */
			if (brrpath_init(&path, arg, brrstr_len(arg, brrpath_max_path))) {
				Err(,"Failed statting argument '%s': %s", arg, brrapi_error_message(nemessage, nemessage_len));
				continue;
			} else if (!path.exists) {
				brrpath_free(&path);
				Err(,"Given input '%s' does not exist", arg);
				continue;
			} else if (path.type != brrpath_type_file) {
				brrpath_free(&path);
				Err(,"Given input '%s' is not a file", arg);
				continue;
			}

			if (state->cfg.next_is_library) {
				if (i_add_library(state, &current, path)) {
					nestate_clear(state);
					return -1;
				}
				state->cfg.next_is_library = 0;
			} else {
				if (i_add_input(state, &current, path)) {
					brrpath_free(&path);
					nestate_clear(state);
					return -1;
				}
				state->cfg.next_is_file = 0;
				working_filter = (nefilter_t){0};
				if (state->cfg.should_reset)
					current = state->default_input;
			}
		}
	}
	state->stats.n_input_digits = brrnum_udigits(state->n_inputs, 10);
	return 0;
}

void
nestate_clear(nestate_t *const state)
{
	if (!state)
		return;
	if (state->inputs) {
		for (brrsz i = 0; i < state->n_inputs; ++i)
			neinput_free(&(state->inputs)[i]);
		free(state->inputs);
	}
	if (state->libraries) {
		for (brrsz i = 0; i < state->n_libraries; ++i)
			neinput_library_free(&(state->libraries)[i]);
		free(state->libraries);
	}
	memset(state, 0, sizeof(*state) - sizeof(state->stats));
}
