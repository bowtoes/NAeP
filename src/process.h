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

#ifndef PROCESS_H
#define PROCESS_H

#include <brrtools/brrapi.h>

#include <brrtools/brrlog.h>
#include <brrtools/brrpath.h>
#include <brrtools/brrstg.h>

BRRCPPSTART

typedef struct input_options {
	brrlog_priorityT log_priority; /* Priority used if logging for the input is enabled. */

	#define INPUT_TYPE_UNK 0x00
	#define INPUT_TYPE_OGG 0x01
	#define INPUT_TYPE_WEM 0x02
	#define INPUT_TYPE_WSP 0x03
	#define INPUT_TYPE_BNK 0x04
	brrby type:3; /* Type of the input.  */

	brrby auto_ogg:1; /* Should output weems automatically be converted to ogg? */
	brrby inplace_ogg:1; /* Should weem-to-ogg conversion be done in-place (replace)? */
	brrby auto_revorb:1; /* Should output oggs be revorbed automatically? */
	brrby inplace_revorb:1; /* Should revorbed oggs replace the original? */
	brrby bank_recurse:1; /* Should input bank files be recursed, searching for referenced bank files? */

	brrby log_enabled:1; /* Is output logging enabled? */
	brrby log_color_enabled:1; /* Is log coloring enabled? */
	brrby log_debug:1; /* Is debug logging enabled (implies log_enabled)? */
	brrby dry_run:1; /* Do not process any input or output, just print what would happen. */
} input_optionsT;
typedef struct input {
	brrstgT input;
	input_optionsT options;
} inputT;

typedef union fourcc {
	struct {
		brru1 _0;
		brru1 _1;
		brru1 _2;
		brru1 _3;
	} bytes;
	brru4 integer;
} fourccT;

extern const fourccT goggfcc;
extern const fourccT gwemfcc;
extern const fourccT gbnkfcc;

void BRRCALL input_delete(inputT *const input);
void BRRCALL input_print(brrlog_priorityT priority, int newline, const inputT *const input, brrsz max_input_length);

inputT *BRRCALL find_argument(const char *const arg, const inputT *const inputs, brrsz input_count);
int BRRCALL parse_argument(void (*const print_help)(void),
    const char *const arg, int *const reset, input_optionsT *const options,
    inputT *const inputs, brrsz input_count, const input_optionsT *const default_options);

/* Returns 0 on success, return negative value on error. */

#define TYPE_ERR_NONE 0
#define TYPE_ERR_INPUT -1
#define TYPE_ERR_READ -2
#define TYPE_ERR_TYPE -3
int BRRCALL determine_type(inputT *const input, const brrpath_infoT *const input_info);

BRRCPPEND

#endif /* PROCESS_H */