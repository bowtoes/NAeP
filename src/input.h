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
} neinput_filter_typeT;

/* TODO Eventually index white/blackisting can be replaced by a more flexible filtering system, similar to in 'countwsp' */
typedef struct neinput_filter {
	brru4 *list;
	brru4 count;
	neinput_filter_typeT type;
} neinput_filterT;

void neinput_filter_clear(neinput_filterT *const filter);
int neinput_filter_contains(const neinput_filterT *const filter, brru4 index);

typedef enum neinput_type {
	neinput_type_auto = 0,
	neinput_type_ogg,
	neinput_type_wem,
	neinput_type_wsp,
	neinput_type_bnk,
} neinput_typeT;
typedef brru1 neinput_type_int;

typedef struct neinput {
	brrsz library_index;              /* Which loaded codebook to use; defaults to 0. */
	const char *path;
	brru2 path_length;
	neinput_type_int type;
	brrlog_priority_int log_priority; /* Priority used if logging is enabled. */
	struct {
		brru1 log_enabled:1;          /* Is output logging enabled? */
		brru1 log_color_enabled:1;    /* Is log coloring enabled? */
		brru1 log_debug:1;            /* Is debug logging enabled (implies log_enabled)? */
		brru1 stripped_headers:1;     /* Whether the vorbis headers of a given wem are stripped or spec-compliant. */
		brru1 dry_run:1;              /* Do not process any input or output, just print what would happen. */
		brru1 auto_ogg:1;             /* Should output weems automatically be converted to ogg? */
		brru1 inplace_ogg:1;          /* Should weem-to-ogg conversion be done in-place (replace)? */
		brru1 inplace_regrain:1;      /* Should regranularized oggs replace the original? */
	} flag;
	neinput_filterT filter;
} neinputT;

void neinput_clear(neinputT *const input);

typedef struct neinput_library {
	struct {
		brru2 loaded:1;      /* Whether the library is valid and ready for use */
		brru2 old:1;         /* Whether to use the old form of deserialization */
		brru2 load_error:14; /* If non-zero, the library failed to be loaded */
	} status;
	brru2 path_length;
	const char *path;
	codebook_libraryT library;
} neinput_libraryT;

int neinput_library_load(neinput_libraryT *const library);
void neinput_library_clear(neinput_libraryT *const library);

typedef struct neprocessstat {
	brrsz assigned;
	brrsz succeeded;
	brrsz failed;
} neprocessstatT;
typedef struct nestate {
	neinputT *inputs;
	brrsz n_inputs;
	const neinputT default_input;
	neinput_libraryT *libraries;
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

		neprocessstatT oggs, wems, wsps, bnks;
		neprocessstatT wem_extracts;
		neprocessstatT wem_converts;
	} stats;
} nestateT;

int nestate_init(nestateT *const state, int argc, char **argv);
void nestate_clear(nestateT *const state);

int neinput_load_codebooks(neinput_libraryT *const libraries, const codebook_libraryT **const library, brrsz index);

#endif /* INPUT_H */
