/* Copyright (c), bowtoes (bow.toes@mailfence.com)
Apache 2.0 license, http://www.apache.org/licenses/LICENSE-2.0
Full license can be found in 'license' file */

#ifndef NAeP_neinput_h
#define NAeP_neinput_h

#include <brrtools/brrlog.h>
#include <brrtools/brrpath.h>

#include "codebook_library.h"
#include "nefilter.h"

/*
 * TODO
 * Only a naive strcmp is done when checking for duplicate files, so one could
 * pass the same file differently and that one file would be allocated and
 * processed twice; this'll be something for brrpath to help with eventually.
 * */

typedef enum nedatatype {
	nedatatype_auto = 0,
	nedatatype_ogg,
	nedatatype_wem,
	nedatatype_wsp,
	nedatatype_bnk,
	nedatatype_arc,
} nedatatype_t;

typedef struct neinput {
	int log_label;                    /* Priority used if logging is enabled. */
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
		brru2 data_type:7;            /* The kind of data this input represents. */
	} cfg;
	brrsz library_index;              /* Which loaded codebook to use; defaults to 0. */
	nefilter_t filter; /* TODO what is a filter */
	brrpath_t path;
} neinput_t;

void
neinput_free(neinput_t *const input);

typedef struct neinput_library {
	brrpath_t path;
	codebook_library_t library;
	struct {
		brru2 loaded:1;      /* Whether the library is valid and ready for use */
		brru2 alternate:1;   /* If this library is stored in the alternate form */
		brru2 load_error;    /* If non-zero, the library failed to be loaded */
	} status;
} neinput_library_t;

int
neinput_library_load(neinput_library_t *const library);

void
neinput_library_free(neinput_library_t *const library);

int
neinput_load_codebooks(neinput_library_t *const libraries, const codebook_library_t **const library, brrsz index);

typedef struct nestate_stat {
	brrsz assigned;
	brrsz succeeded;
	brrsz failed;
} nestate_stat_t;

typedef struct nestate {
	const neinput_t default_input;
	neinput_t *inputs;
	brrsz n_inputs;
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
	} cfg;

	struct {
		brrsz input_path_max; /* For log padding */
		brrsz n_input_digits;

		nestate_stat_t oggs, wems, wsps, bnks;
		nestate_stat_t wem_extracts;
		nestate_stat_t wem_converts;
	} stats;
} nestate_t;

int
nestate_init(nestate_t *const state, int argc, char **argv);

/* Does not zero-out stats */
void
nestate_clear(nestate_t *const state);

void
nestate_reset(nestate_t *const state);

#endif /* NAeP_neinput_h */
