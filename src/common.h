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

#include <brrtools/brrlog.h>
#include <brrtools/brrtypes.h>

#if defined(BRRTOOLS_BRRLOG_H)
# define NeTODO(...) do { \
	brrlog_formatT _tf_ = gbrrlog_format_last; \
	brrlog_levelT _lf_ = gbrrlog_level_last; \
	BRRLOG_DEBUGNP(""); \
	BRRLOG_FORENP(brrlog_color_green, " TODO:"); \
	BRRLOG_FORENP(brrlog_color_normal, __VA_ARGS__); \
	gbrrlog_format_last = _tf_; \
	gbrrlog_level_last = _lf_; \
} while (0)
#endif /* BRRTOOLS_BRRLOG_H */

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

typedef union fourcc {
	struct {
		brru1 _0;
		brru1 _1;
		brru1 _2;
		brru1 _3;
	} bytes;
	brru4 integer;
} fourccT;

#define _init_fcc(_a_, _b_, _c_, _d_) {.bytes={(_a_), (_b_), (_c_), (_d_)}}
#define _fcc_lit(_l_) _init_fcc((_l_)[0], (_l_)[1], (_l_)[2], (_l_)[3])
#define MAKE_FCC(_l_) _fcc_lit(#_l_)
#define GET_FCC_BYTES(_f_) (_f_).bytes._0, (_f_).bytes._1, (_f_).bytes._2, (_f_).bytes._3

extern const fourccT goggfcc;
extern const fourccT gwemfcc;
extern const fourccT gbnkfcc;

typedef struct numbers {
	brrsz paths_count;
	brrsz path_maximum_length; /* For log padding */

	brrsz bnks_to_process, bnks_processed;
	brrsz wsps_to_process, wsps_processed;
	brrsz wems_extracted;
	brrsz wems_to_convert, wems_converted;
	brrsz oggs_to_regranularize, oggs_regranularized;
} numbersT;

/* Compares 'a' and 'b' with case-(in)sensitive versions of 'strcmp' and
 * returns the result.
 * If 'max_length' is greater than 0, uses max-length versions of 'strcmp'
 * */
int BRRCALL cstr_compare(const char *const a, const char *const b, brrsz max_length, int case_sensitive);

#endif /* COMMON_H */
