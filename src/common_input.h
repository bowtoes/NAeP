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

#ifndef COMMON_INPUT_H
#define COMMON_INPUT_H

#include <brrtools/brrapi.h>
#include <brrtools/brrlog.h>
#include <brrtools/brrstg.h>
#include <brrtools/brrtypes.h>

#include "codebook_library.h"

BRRCPPSTART

#define LOG_FORMAT_OGG ((brrlog_formatT){brrlog_color_blue,    -1, -1, -1})
#define LOG_COLOR_OGG  LOG_FORMAT_OGG.foreground
#define LOG_BGCOL_OGG  LOG_FORMAT_OGG.background
#define LOG_STYLE_OGG  LOG_FORMAT_OGG.style
#define LOG_FONT_OGG   LOG_FORMAT_OGG.font

#define LOG_FORMAT_WEM ((brrlog_formatT){brrlog_color_green,   -1, -1, -1})
#define LOG_COLOR_WEM  LOG_FORMAT_WEM.foreground
#define LOG_BGCOL_WEM  LOG_FORMAT_WEM.background
#define LOG_STYLE_WEM  LOG_FORMAT_WEM.style
#define LOG_FONT_WEM   LOG_FORMAT_WEM.font

#define LOG_FORMAT_WSP ((brrlog_formatT){brrlog_color_yellow,  -1, -1, -1})
#define LOG_COLOR_WSP  LOG_FORMAT_WSP.foreground
#define LOG_BGCOL_WSP  LOG_FORMAT_WSP.background
#define LOG_STYLE_WSP  LOG_FORMAT_WSP.style
#define LOG_FONT_WSP   LOG_FORMAT_WSP.font

#define LOG_FORMAT_BNK ((brrlog_formatT){brrlog_color_red,     -1, -1, -1})
#define LOG_COLOR_BNK  LOG_FORMAT_BNK.foreground
#define LOG_BGCOL_BNK  LOG_FORMAT_BNK.background
#define LOG_STYLE_BNK  LOG_FORMAT_BNK.style
#define LOG_FONT_BNK   LOG_FORMAT_BNK.font

#define LOG_FORMAT_AUT ((brrlog_formatT){brrlog_color_magenta, -1, brrlog_style_bold, -1})
#define LOG_COLOR_AUT  LOG_FORMAT_AUT.foreground
#define LOG_BGCOL_AUT  LOG_FORMAT_AUT.background
#define LOG_STYLE_AUT  LOG_FORMAT_AUT.style
#define LOG_FONT_AUT   LOG_FORMAT_AUT.font

#define LOG_COLOR_DRY brrlog_color_magenta
#define LOG_COLOR_WET brrlog_color_cyan

#define LOG_COLOR_MANUAL   brrlog_color_red
#define LOG_COLOR_INPLACE  brrlog_color_yellow
#define LOG_COLOR_SEPARATE brrlog_color_cyan
#define LOG_COLOR_ENABLED  brrlog_color_green
#define LOG_COLOR_DISABLED brrlog_color_red

#define LOG_FORMAT_SUCCESS ((brrlog_formatT){brrlog_color_green, -1, brrlog_style_bold, -1})
#define LOG_COLOR_SUCCESS  LOG_FORMAT_SUCCESS.foreground
#define LOG_BGCOL_SUCCESS  LOG_FORMAT_SUCCESS.background
#define LOG_STYLE_SUCCESS  LOG_FORMAT_SUCCESS.style
#define LOG_FONT_SUCCESS   LOG_FORMAT_SUCCESS.font

#define LOG_FORMAT_FAILURE ((brrlog_formatT){brrlog_color_red, -1, brrlog_style_bold, -1})
#define LOG_COLOR_FAILURE  LOG_FORMAT_FAILURE.foreground
#define LOG_BGCOL_FAILURE  LOG_FORMAT_FAILURE.background
#define LOG_STYLE_FAILURE  LOG_FORMAT_FAILURE.style
#define LOG_FONT_FAILURE   LOG_FORMAT_FAILURE.font

#define LOG_COLOR_PATH brrlog_color_cyan
#define LOG_COLOR_INFO brrlog_color_magenta

typedef struct numbers {
	brrsz n_inputs;
	brrsz n_libraries;
	brrsz input_path_max_length; /* For log padding */

	brrsz oggs_to_regrain, oggs_regrained, oggs_failed;
	brrsz wems_to_convert, wems_converted, wems_failed;
	brrsz wsps_to_process, wsps_processed, wsps_failed;
	brrsz bnks_to_process, bnks_processed, bnks_failed;
	brrsz wems_to_extract, wems_extracted;
	brrsz wems_to_convert_extract, wems_convert_extracted;
} numbersT;
typedef struct global_options {
	brru1 should_reset:1;
	brru1 log_style_enabled:1;
	brru1 next_is_file:1;
	brru1 always_file:1;
	brru1 next_is_cbl:1;
	brru1 report_card:1;
	brru1 full_report:1;
	brru1 _pad:1;
	/* < Byte boundary > */
} global_optionsT;
typedef struct input_options {
	brrsz library_index;       /* Which loaded codebook to use; defaults to 0. */
	brrlog_priorityT log_priority; /* Priority used if logging for the input is enabled. */

	brru1 type:3;              /* Type of the input.  */
	brru1 auto_ogg:1;          /* Should output weems automatically be converted to ogg? */
	brru1 inplace_ogg:1;       /* Should weem-to-ogg conversion be done in-place (replace)? */
	brru1 inplace_regrain:1;   /* Should regranularized oggs replace the original? */
	brru1 bank_recurse:1;      /* Should input bank files be recursed, searching for referenced bank files? */
	brru1 stripped_headers:1;  /* Whether the vorbis headers of a given wem are stripped or spec-compliant. */
	/* TODO stripped_headers might be automatically
	 * determinable from the given wem; no idea where that'd be */
	/* < Byte boundary > */
	brru1 log_enabled:1;       /* Is output logging enabled? */
	brru1 log_color_enabled:1; /* Is log coloring enabled? */
	brru1 log_debug:1;         /* Is debug logging enabled (implies log_enabled)? */
	brru1 dry_run:1;           /* Do not process any input or output, just print what would happen. */
} input_optionsT;
typedef struct input_library {
	brrstgT library_path;
	codebook_libraryT library;

	brru1 loaded:1; /* Whether the library is valid and ready for use */
	brru1 old:1;    /* Whether to use the old form of deserialization */
	int load_error; /* Whether the library failed to be initialized */
} input_libraryT;
typedef struct input {
	brrstgT path;
	input_optionsT options;
} inputT;

typedef enum input_type {
	input_type_auto = 0,
	input_type_ogg,
	input_type_wem,
	input_type_wsp,
	input_type_bnk,
} input_typeT;

int BRRCALL input_library_load(input_libraryT *const library);
void BRRCALL input_library_clear(input_libraryT *const library);

void BRRCALL input_clear(inputT *const input);
void BRRCALL input_print(const inputT *const input, brrsz max_input_length,
    brrlog_priorityT priority, int newline);

BRRCPPEND

#endif /* COMMON_INPUT_H */
