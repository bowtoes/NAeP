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

#ifndef INPUT_H
#define INPUT_H

#include <brrtools/brrtypes.h>
#include "codebook_library.h"

/*
 * TODO
 * Only a naive strcmp is done when checking for duplicate files, so one could
 * pass the same file differently and that one file would be allocated and
 * processed twice; this'll be something for brrpath to help with eventually.
 * */

typedef enum neinput_type {
	neinput_type_auto = 0,
	neinput_type_ogg,
	neinput_type_wem,
	neinput_type_wsp,
	neinput_type_bnk,
} neinput_typeT;
typedef struct neinput {
	const char *path;
	neinput_typeT type;
	brrsz library_index;       /* Which loaded codebook to use; defaults to 0. */
	int log_priority; /* Priority used if logging for the input is enabled. */

	brru1 auto_ogg:1;          /* Should output weems automatically be converted to ogg? */
	brru1 inplace_ogg:1;       /* Should weem-to-ogg conversion be done in-place (replace)? */
	brru1 inplace_regrain:1;   /* Should regranularized oggs replace the original? */
	brru1 bank_recurse:1;      /* Should input bank files be recursed, searching for referenced bank files? */
	brru1 stripped_headers:1;  /* Whether the vorbis headers of a given wem are stripped or spec-compliant. */
	brru1 log_enabled:1;       /* Is output logging enabled? */
	brru1 log_color_enabled:1; /* Is log coloring enabled? */
	brru1 log_debug:1;         /* Is debug logging enabled (implies log_enabled)? */
	/* <byte> */
	brru1 dry_run:1;           /* Do not process any input or output, just print what would happen. */
} neinputT;
typedef struct neinput_library {
	const char *library_path;
	codebook_libraryT library;

	int loaded; /* Whether the library is valid and ready for use */
	int old;    /* Whether to use the old form of deserialization */
	int load_error; /* If non-zero, the library failed to be loaded */
} neinput_libraryT;
typedef struct nestate {
	brrsz n_inputs;
	brrsz n_libraries;
	brrsz input_path_max; /* For log padding */
	brrsz n_input_digits;

	brrsz oggs_to_regrain, oggs_regrained, oggs_failed;
	brrsz wems_to_convert, wems_converted, wems_failed;
	brrsz wsps_to_process, wsps_processed, wsps_failed;
	brrsz bnks_to_process, bnks_processed, bnks_failed;
	brrsz wems_to_extract, wems_extracted;
	brrsz wems_to_convert_extract, wems_convert_extracted;

	/* Options */
	brru1 should_reset:1;
	brru1 log_style_enabled:1;
	brru1 next_is_file:1;
	brru1 always_file:1;
	brru1 next_is_library:1;
	brru1 report_card:1;
	brru1 full_report:1;
	brru1 _pad:1;
	/* < Byte boundary > */
} nestateT;

/* -1 : Not found */
int neinput_check_extension(const char *const arg, int case_sensitive, ...);

int neinput_take_inputs(nestateT *const state, neinput_libraryT **const libraries,
    neinputT **const inputs, const neinputT default_input, int argc, char **argv);

void neinput_clear(neinputT *const input);

int neinput_library_load(neinput_libraryT *const library);
void neinput_library_clear(neinput_libraryT *const library);

void neinput_clear_all(const nestateT *const state, neinput_libraryT **const libraries, neinputT **const inputs);

#endif /* INPUT_H */
