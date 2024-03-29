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

#ifndef INPUT_H
#define INPUT_H

#include <brrtools/brrtypes.h>
#include <brrtools/brrlog.h>

#include "codebook_library.h"

/*
 * TODO
 * Only a naive strcmp is done when checking for duplicate files, so one could
 * pass the same file differently and that one file would be allocated and
 * processed twice; this'll be something for brrpath to help with eventually.
 * */

typedef enum neinput_filter_type {
	neinput_filter_white = 0,
	neinput_filter_black,
} neinput_filter_type_t;

/* TODO Eventually index white/blackisting can be replaced by a more flexible filtering system, similar to in 'countwsp' */
typedef struct neinput_filter {
	brru4 *list;
	brru4 count;
	neinput_filter_type_t type;
} neinput_filter_t;

void neinput_filter_clear(neinput_filter_t *const filter);
int neinput_filter_contains(const neinput_filter_t *const filter, brru4 index);

typedef enum neinput_type {
	neinput_type_auto = 0,
	neinput_type_ogg,
	neinput_type_wem,
	neinput_type_wsp,
	neinput_type_bnk,
} neinput_type_t;
typedef brru1 neinput_type_int;

typedef struct neinput {
	brrsz library_index;              /* Which loaded codebook to use; defaults to 0. */
	const char *path;
	brru2 path_length;
	neinput_type_int type;
	brrlog_priority_int log_priority; /* Priority used if logging is enabled. */
	struct {
		brru2 log_enabled:1;          /* Is output logging enabled? */
		brru2 log_color_enabled:1;    /* Is log coloring enabled? */
		brru2 log_debug:1;            /* Is debug logging enabled (implies log_enabled)? */
		brru2 add_comments:1;         /* Enables additional comments in output Oggs. */
		brru2 stripped_headers:1;     /* Whether the vorbis headers of a given wem are stripped or spec-compliant. */
		brru2 dry_run:1;              /* Do not process any input or output, just print what would happen. */
		brru2 auto_ogg:1;             /* Should output weems automatically be converted to ogg? */
		brru2 inplace_ogg:1;          /* Should weem-to-ogg conversion be done in-place (replace)? */
		brru2 inplace_regrain:1;      /* Should regranularized oggs replace the original? */
	} flag;
	neinput_filter_t filter;
} neinput_t;

void neinput_clear(neinput_t *const input);

typedef struct neinput_library {
	struct {
		brru2 loaded:1;      /* Whether the library is valid and ready for use */
		brru2 old:1;         /* Whether to use the old form of deserialization */
		brru2 load_error:14; /* If non-zero, the library failed to be loaded */
	} status;
	brru2 path_length;
	const char *path;
	codebook_library_t library;
} neinput_library_t;

int neinput_library_load(neinput_library_t *const library);
void neinput_library_clear(neinput_library_t *const library);
int neinput_load_codebooks(neinput_library_t *const libraries, const codebook_library_t **const library, brrsz index);

typedef struct neprocessstat {
	brrsz assigned;
	brrsz succeeded;
	brrsz failed;
} nestate_stat_t;
typedef struct nestate {
	neinput_t *inputs;
	brrsz n_inputs;
	const neinput_t default_input;
	neinput_library_t *libraries;
	brrsz n_libraries;

	struct {
		brru8 next_is_file:1;
		brru8 next_is_library:1;
		brru8 next_is_filter:1;
		brru8 always_file:1;
		brru8 should_reset:1;
		brru8 log_style_enabled:1;
		brru8 report_card:1;
		brru8 full_report:1;
	/* < Byte boundary > */
	} settings;

	struct {
		brrsz input_path_max; /* For log padding */
		brrsz n_input_digits;

		nestate_stat_t oggs, wems, wsps, bnks;
		nestate_stat_t wem_extracts;
		nestate_stat_t wem_converts;
	} stats;
} nestate_t;

int nestate_init(nestate_t *const state, int argc, char **argv);
void nestate_clear(nestate_t *const state);

#endif /* INPUT_H */
