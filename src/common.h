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

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>

#include <brrtools/brrapi.h>
#include <brrtools/brrlog.h>
#include <brrtools/brrstg.h>
#include <brrtools/brrtypes.h>
#include <brrtools/brrplatform.h>

#include "fcc.h"
#include "codebook_library.h"

#define NeFUNC(...) do { BRRLOG_DEBUGN("'%s' (%s:%zu)    ", __func__, __FILE__, __LINE__); BRRLOG_DEBUGP(__VA_ARGS__); } while (0)
# define NeTODO(...) do { \
	brrlog_formatT _tf_ = gbrrlog_format_last; \
	brrlog_levelT _lf_ = gbrrlog_level_last; \
	BRRLOG_DEBUGNP(""); \
	BRRLOG_FORENP(brrlog_color_green, " TODO:"); \
	BRRLOG_FORENP(brrlog_color_normal, __VA_ARGS__); \
	gbrrlog_format_last = _tf_; \
	gbrrlog_level_last = _lf_; \
} while (0)

#define OGG_FORMAT ((brrlog_formatT){brrlog_color_blue,    -1, -1, -1})
#define OGG_COLOR OGG_FORMAT.foreground
#define OGG_BGCOL OGG_FORMAT.background
#define OGG_STYLE OGG_FORMAT.style
#define OGG_FONT OGG_FORMAT.font

#define WEM_FORMAT ((brrlog_formatT){brrlog_color_green,   -1, -1, -1})
#define WEM_COLOR WEM_FORMAT.foreground
#define WEM_BGCOL WEM_FORMAT.background
#define WEM_STYLE WEM_FORMAT.style
#define WEM_FONT WEM_FORMAT.font

#define WSP_FORMAT ((brrlog_formatT){brrlog_color_yellow,  -1, -1, -1})
#define WSP_COLOR WSP_FORMAT.foreground
#define WSP_BGCOL WSP_FORMAT.background
#define WSP_STYLE WSP_FORMAT.style
#define WSP_FONT WSP_FORMAT.font

#define BNK_FORMAT ((brrlog_formatT){brrlog_color_red,     -1, -1, -1})
#define BNK_COLOR BNK_FORMAT.foreground
#define BNK_BGCOL BNK_FORMAT.background
#define BNK_STYLE BNK_FORMAT.style
#define BNK_FONT BNK_FORMAT.font

#define AUT_FORMAT ((brrlog_formatT){brrlog_color_magenta, -1, brrlog_style_bold, -1})
#define AUT_COLOR AUT_FORMAT.foreground
#define AUT_BGCOL AUT_FORMAT.background
#define AUT_STYLE AUT_FORMAT.style
#define AUT_FONT AUT_FORMAT.font

#define DRY_COLOR brrlog_color_magenta
#define WET_COLOR brrlog_color_cyan

#define MANUAL_COLOR brrlog_color_red
#define INPLACE_COLOR brrlog_color_yellow
#define SEPARATE_COLOR brrlog_color_cyan
#define ENABLED_COLOR brrlog_color_green
#define DISABLED_COLOR brrlog_color_red

#define SUCCESS_FORMAT ((brrlog_formatT){brrlog_color_green, -1, brrlog_style_bold, -1})
#define SUCCESS_COLOR SUCCESS_FORMAT.foreground
#define SUCCESS_BGCOL SUCCESS_FORMAT.background
#define SUCCESS_STYLE SUCCESS_FORMAT.style
#define SUCCESS_FONT SUCCESS_FORMAT.font

#define FAILURE_FORMAT ((brrlog_formatT){brrlog_color_red, -1, brrlog_style_bold, -1})
#define FAILURE_COLOR FAILURE_FORMAT.foreground
#define FAILURE_BGCOL FAILURE_FORMAT.background
#define FAILURE_STYLE FAILURE_FORMAT.style
#define FAILURE_FONT FAILURE_FORMAT.font

#define PATH_COLOR brrlog_color_cyan
#define INFO_COLOR brrlog_color_magenta

/* Common internal return codes */
#define I_SUCCESS 0
#define I_BUFFER_ERROR -1
#define I_IO_ERROR -2
#define I_FILE_TRUNCATED -3
#define I_INIT_ERROR -4
#define I_NOT_VORBIS -5
#define I_DESYNC -6
#define I_CORRUPT -7
#define I_NOT_RIFF -8
#define I_UNRECOGNIZED_DATA -9
#define I_BAD_ERROR -99

/* Ogg/Vorbis function return codes */
#define SYNC_PAGEOUT_SUCCESS 1
#define SYNC_PAGEOUT_INCOMPLETE 0
#define SYNC_PAGEOUT_DESYNC -1
#define SYNC_WROTE_SUCCESS 0
#define SYNC_WROTE_FAILURE -1
#define STREAM_INIT_SUCCESS 0
#define STREAM_PAGEIN_SUCCESS 0
#define STREAM_PACKETIN_SUCCESS 0
#define STREAM_PACKETOUT_SUCCESS 1
#define STREAM_PACKETOUT_INCOMPLETE 0
#define STREAM_PACKETOUT_DESYNC -1
#define VORBIS_SYNTHESIS_HEADERIN_SUCCESS 0
#define VORBIS_SYNTHESIS_HEADERIN_FAULT OV_EFAULT
#define VORBIS_SYNTHESIS_HEADERIN_NOTVORBIS OV_ENOTVORBIS
#define VORBIS_SYNTHESIS_HEADERIN_BADHEADER OV_EBADHEADER

BRRCPPSTART

extern const fourccT goggfcc;
extern const fourccT gwemfcc;
extern const fourccT gbnkfcc;

const char *BRRCALL i_strerr(int err);

typedef struct numbers {
	brrsz n_inputs;
	brrsz n_libraries;
	brrsz input_path_max_length; /* For log padding */

	brrsz bnks_to_process, bnks_processed;
	brrsz wsps_to_process, wsps_processed;
	brrsz wems_extracted;
	brrsz wems_to_convert, wems_converted;
	brrsz oggs_to_regranularize, oggs_regranularized;
} numbersT;
typedef struct global_options {
	brru1 should_reset:1;
	brru1 log_style_enabled:1;
	brru1 next_is_file:1;
	brru1 always_file:1;
	brru1 next_is_cbl:1;
	brru1 _pad:3;
	/* < Byte boundary > */
} global_optionsT;
typedef struct input_options {
	brrsz library_index; /* Which loaded codebook to use; defaults to 0. */
	brrlog_priorityT log_priority; /* Priority used if logging for the input is enabled. */

	#define INPUT_TYPE_UNK 0x00
	#define INPUT_TYPE_OGG 0x01
	#define INPUT_TYPE_WEM 0x02
	#define INPUT_TYPE_WSP 0x03
	#define INPUT_TYPE_BNK 0x04
	brru1 type:3; /* Type of the input.  */

	brru1 auto_ogg:1; /* Should output weems automatically be converted to ogg? */
	brru1 inplace_ogg:1; /* Should weem-to-ogg conversion be done in-place (replace)? */
	brru1 inplace_regranularize:1; /* Should regranularized oggs replace the original? */
	brru1 bank_recurse:1; /* Should input bank files be recursed, searching for referenced bank files? */

	brru1 log_enabled:1; /* Is output logging enabled? */
	/* < Byte boundary > */
	brru1 log_color_enabled:1; /* Is log coloring enabled? */
	brru1 log_debug:1; /* Is debug logging enabled (implies log_enabled)? */
	brru1 dry_run:1; /* Do not process any input or output, just print what would happen. */
} input_optionsT;
typedef struct input_library {
	brrstgT library_path;
	codebook_libraryT library;

	brru1 loaded:1; /* Whether the library is valid and ready for use */
	brru1 old:1; /* Whether to use the old form of deserialization */
	int failed; /* Whether the library failed to be initialized */
} input_libraryT;
typedef struct processed_input {
	brrstgT path;
	input_optionsT options;
} processed_inputT;

int BRRCALL input_library_load(input_libraryT *const library);
void BRRCALL input_library_clear(input_libraryT *const library);

void BRRCALL processed_input_clear(processed_inputT *const input);
void BRRCALL processed_input_print(const processed_inputT *const input, brrsz max_input_length,
    brrlog_priorityT priority, int newline);

int BRRCALL replace_ext(const char *const input,
    brrsz * const inlen, char *const output, brrsz *const outlen,
    const char *const replacement);

/* Compares 'a' and 'b' with case-(in)sensitive versions of 'strcmp' and
 * returns the result.
 * If 'max_length' is greater than 0, uses max-length versions of 'strcmp'
 * */
int BRRCALL cstr_compare(const char *const a, const char *const b,
    brrsz max_length, int case_sensitive);

BRRCPPEND

#endif /* COMMON_H */
