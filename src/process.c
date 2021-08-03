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

#include "process.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <brrtools/brrlib.h>
#include <brrtools/brrmem.h>
#include <brrtools/brrtil.h>
#include <brrtools/brrpath.h>

#include "process_files.h"

void BRRCALL
input_delete(inputT *const input)
{
	if (input) {
		brrstg_delete(&input->path);
		input->options = (input_optionsT){0};
	}
}
void BRRCALL
input_print(brrlog_priorityT priority, int newline, const inputT *const input, brrsz max_input_length)
{
	gbrrlog_level_last = gbrrlog_level_debug;
	gbrrlog_format_last = gbrrlog_format_normal;
	BRRLOG_LASTNP(" ");
	if (input->options.type == INPUT_TYPE_OGG)      { BRRLOG_MESSAGETNP(gbrrlog_level_last, OGG_FORMAT, "OGG"); }
	else if (input->options.type == INPUT_TYPE_WEM) { BRRLOG_MESSAGETNP(gbrrlog_level_last, WEM_FORMAT, "WEM"); }
	else if (input->options.type == INPUT_TYPE_WSP) { BRRLOG_MESSAGETNP(gbrrlog_level_last, WSP_FORMAT, "WSP"); }
	else if (input->options.type == INPUT_TYPE_BNK) { BRRLOG_MESSAGETNP(gbrrlog_level_last, BNK_FORMAT, "BNK"); }
	else                                            { BRRLOG_MESSAGETNP(gbrrlog_level_last, AUT_FORMAT, "AUT"); }

	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " log ");
	if (input->options.log_enabled) { BRRLOG_STYLENP(ENABLED_COLOR, -1, brrlog_style_bold, "ENB"); }
	else { BRRLOG_FORENP(DISABLED_COLOR, "DSB"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " ");
	if (input->options.log_color_enabled) { BRRLOG_STYLENP(ENABLED_COLOR, -1, brrlog_style_bold, "STY"); }
	else { BRRLOG_FORENP(DISABLED_COLOR, "SMP"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " ");
	if (input->options.log_debug) { BRRLOG_FORENP(brrlog_color_cyan, "DBG"); }
	else { BRRLOG_FORENP(brrlog_color_yellow, "NRM"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " ");
	BRRLOG_FORENP(brrlog_color_normal + 1 + input->options.log_priority,
	    "%s", brrlog_priority_dbgstr(input->options.log_priority));
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " operation ");
	if (input->options.dry_run) { BRRLOG_FORENP(DRY_COLOR, "DRY"); }
	else { BRRLOG_FORENP(WET_COLOR, "WET"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " w2o ");
	if (input->options.auto_ogg) { BRRLOG_FORENP(AUT_COLOR, "AUT"); }
	else { BRRLOG_FORENP(MANUAL_COLOR, "MAN"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " ");
	if (input->options.inplace_ogg) { BRRLOG_FORENP(INPLACE_COLOR, "INP"); }
	else { BRRLOG_FORENP(SEPARATE_COLOR, "SEP"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " rvb ");
	if (input->options.auto_ogg) { BRRLOG_FORENP(AUT_COLOR, "AUT"); }
	else { BRRLOG_FORENP(MANUAL_COLOR, "MAN"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " ");
	if (input->options.inplace_ogg) { BRRLOG_FORENP(INPLACE_COLOR, "INP"); }
	else { BRRLOG_FORENP(MANUAL_COLOR, "SEP"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " bank recurse ");
	if (input->options.bank_recurse) { BRRLOG_FORENP(ENABLED_COLOR, "FUL"); }
	else { BRRLOG_FORENP(DISABLED_COLOR, "NON"); }
	if (newline)
		BRRLOG_LASTP("");
}

inputT *BRRCALL
find_argument(const char *const arg, const inputT *const inputs, brrsz input_count)
{
	if (!inputs || !input_count || !arg) {
		return NULL;
	} else {
		for (brrsz i = 0; i < input_count; ++i) {
			if (0 == strcmp(inputs[i].path.opaque, arg))
				return (inputT *)&inputs[i];
		}
		return NULL;
	}
}
int BRRCALL
parse_argument(void (*const print_help)(void),
    const char *const arg, global_optionsT *const global,
    input_optionsT *const options, inputT *const inputs, brrsz input_count,
    const input_optionsT *const default_options)
{
	inputT *temp = NULL;
#define IF_CHECK_ARG(_cse_, ...) if (-1 != brrstg_cstr_compare(arg, _cse_, __VA_ARGS__, NULL))
	IF_CHECK_ARG(0, "-h", "-help", "--help", "-v", "-version", "--version") {
		print_help();
	} else IF_CHECK_ARG(1, "-a", "-auto", "-detect") {
		options->type = INPUT_TYPE_UNK; return 1;
	} else IF_CHECK_ARG(1, "-o", "-ogg") {
		options->type = INPUT_TYPE_OGG; return 1;
	} else IF_CHECK_ARG(1, "-w", "-wem", "-weem") {
		options->type = INPUT_TYPE_WEM; return 1;
	} else IF_CHECK_ARG(1, "-W", "-wsp", "-wisp") {
		options->type = INPUT_TYPE_WSP; return 1;
	} else IF_CHECK_ARG(1, "-b", "-bnk", "-bank") {
		options->type = INPUT_TYPE_BNK; return 1;
	} else IF_CHECK_ARG(1, "-R", "-recurse-bank") {
		BRRTIL_TOGGLE(options->bank_recurse); return 1;
	} else IF_CHECK_ARG(1, "-O", "-weem2ogg") {
		BRRTIL_TOGGLE(options->auto_ogg); return 1;
	} else IF_CHECK_ARG(1, "-oi", "-ogg-inplace") {
		BRRTIL_TOGGLE(options->inplace_ogg); return 1;
	} else IF_CHECK_ARG(1, "-r", "-revorb") {
		BRRTIL_TOGGLE(options->auto_revorb); return 1;
	} else IF_CHECK_ARG(1, "-ri", "-rvb-inplace") {
		BRRTIL_TOGGLE(options->inplace_revorb); return 1;
	} else IF_CHECK_ARG(1, "-Q", "-qq", "-too-quiet") {
		BRRTIL_TOGGLE(options->log_enabled); return 1;
	} else IF_CHECK_ARG(1, "-c", "-color") {
		BRRTIL_TOGGLE(options->log_color_enabled); return 1;
	} else IF_CHECK_ARG(1, "-C", "-global_color") {
		BRRTIL_TOGGLE((*global).log_style_enabled); return 1;
	} else IF_CHECK_ARG(1, "-d", "-debug") {
		BRRTIL_TOGGLE(options->log_debug);
		if (options->log_debug)
			options->log_enabled = 1;
		return 1;
	} else IF_CHECK_ARG(1, "-q", "-quiet") {
		if (options->log_priority > 0)
			options->log_priority--;
		if (options->log_priority == 0 && !options->log_debug)
			options->log_enabled = 0;
		return 1;
	} else IF_CHECK_ARG(1, "+q", "+quiet") {
		if (options->log_priority < brrlog_priority_count - 1)
			options->log_priority++;
		options->log_enabled = 1;
		return 1;
	} else IF_CHECK_ARG(1, "-n", "-dry", "-dryrun") {
		BRRTIL_TOGGLE(options->dry_run); return 1;
	} else IF_CHECK_ARG(1, "-reset") {
		BRRTIL_TOGGLE((*global).should_reset); return 1;
	} else if ((temp = find_argument(arg, inputs, input_count))) {
		temp->options = *options; *options = *default_options; return 1;
	}
#undef IF_CHECK_ARG
	return 0;
}

#define TYPE_ERR_NONE 0
#define TYPE_ERR_INPUT -1
#define TYPE_ERR_READ -2
#define TYPE_ERR_TYPE -3
/* Returns 0 on success, return negative value on error. */
static int BRRCALL
determine_type(inputT *const input, const char *const extension)
{
	int err = 0;
	fourccT input_fcc = {0};
	FILE *fp = NULL;
	if (!(fp = fopen((char *)input->path.opaque, "rb"))) {
		return TYPE_ERR_INPUT;
	} else if (4 != fread(&input_fcc.integer, 1, 4, fp)) {
		if (feof(fp)) {
			fclose(fp);
			if (!brrstg_cstr_compare(extension, 0, "ogg", NULL)) {
				input->options.type = INPUT_TYPE_OGG;
			} else if (!brrstg_cstr_compare(extension, 0, "wem", NULL)) {
				input->options.type = INPUT_TYPE_WEM;
			} else if (!brrstg_cstr_compare(extension, 0, "wsp", NULL)) {
				input->options.type = INPUT_TYPE_WSP;
			} else if (!brrstg_cstr_compare(extension, 0, "bnk", NULL)) {
				input->options.type = INPUT_TYPE_BNK;
			} else {
				err = TYPE_ERR_TYPE;
			}
			if (!err) {
				BRRLOG_WARN("File is less than 4 bytes long ");
			}
			return err;
		} else {
			err = TYPE_ERR_READ;
		}
	}
	fclose(fp);
	if (!err) {
		BRRLOG_DEBUGNP(" FCC %08X = %02X %02X %02X %02X", input_fcc, GET_FCC_BYTES(input_fcc));
		if (input_fcc.integer == goggfcc.integer) {
			input->options.type = INPUT_TYPE_OGG;
		} else if (input_fcc.integer == gwemfcc.integer) {
			if (!brrstg_cstr_compare(extension, 0, "wem", NULL))
				input->options.type = INPUT_TYPE_WEM;
			else if (!brrstg_cstr_compare(extension, 0, "wsp", NULL))
				input->options.type = INPUT_TYPE_WSP;
			else
				return TYPE_ERR_TYPE;
		} else if (input_fcc.integer == gbnkfcc.integer) {
			input->options.type = INPUT_TYPE_BNK;
		} else {
			err = TYPE_ERR_TYPE;
		}
	}
	return err;
}

int BRRCALL
process_input(inputT *const input, numbersT *const numbers,
    const brrpath_stat_resultT *const path_stat, brrsz index)
{
	int err = 0;
	brrsz input_count_digits = brrlib_ndigits(numbers->paths_count, 0, 10);
	brrstgT directory = {0}, base_name = {0}, extension = {0};
	/* TODO Check if can open file */
	if (brrpath_split(&directory, &base_name, &extension, input->path.opaque)) {
		return -1;
	}
	BRRLOG_NORN("Parsing");
	BRRLOG_FORENP(INFO_COLOR, " %*i / %*i ",
	    input_count_digits, index + 1, input_count_digits, numbers->paths_count);
#if !defined(NeDEBUG)
	if (input->options.log_debug) {
#endif
		input_print(brrlog_priority_debug, 0, input, numbers->path_maximum_length);
#if !defined(NeDEBUG)
	}
#endif
	BRRLOG_NORNP(" "); /* Reset last log format and level */
	if (input->options.type == INPUT_TYPE_UNK) {
		int err = 0;
		if ((err = determine_type(input, extension.opaque))) {
			BRRLOG_ERRN("Failed to determine filetype for ");
			BRRLOG_FORENP(PATH_COLOR, "%-*s", numbers->path_maximum_length, BRRTIL_NULSTR((char *)input->path.opaque));
			if (err == TYPE_ERR_INPUT)
				BRRLOG_ERRP(" : Could not open file for reading (%s)", strerror(errno));
			else if (err == TYPE_ERR_READ)
				BRRLOG_ERRP(" : Failed to read four-character-code of file : %s", strerror(errno));
			else
				BRRLOG_ERRP(" : File has no recognized type");
		}
	}
	if (!err) {
		BRRLOG_NORNP(""); /* Reset last log format and level */
		if (input->options.type == INPUT_TYPE_OGG) {
			BRRLOG_MESSAGETNP(gbrrlog_level_last, OGG_FORMAT, "%-*s",
			    numbers->path_maximum_length, BRRTIL_NULSTR((char *)input->path.opaque));
			revorb_ogg(numbers, input->options.dry_run, input->path.opaque,
			    input->options.inplace_revorb);
		} else if (input->options.type == INPUT_TYPE_WEM) {
			BRRLOG_MESSAGETNP(gbrrlog_level_last, WEM_FORMAT, "%-*s",
			    numbers->path_maximum_length, BRRTIL_NULSTR((char *)input->path.opaque));
			convert_wem(numbers, input->options.dry_run, input->path.opaque,
			    input->options.inplace_revorb, input->options.auto_revorb,
			    input->options.inplace_ogg);
		} else if (input->options.type == INPUT_TYPE_WSP) {
			BRRLOG_MESSAGETNP(gbrrlog_level_last, WSP_FORMAT, "%-*s",
			    numbers->path_maximum_length, BRRTIL_NULSTR((char *)input->path.opaque));
			extract_wsp(numbers, input->options.dry_run, input->path.opaque,
			    input->options.inplace_revorb, input->options.auto_revorb,
			    input->options.inplace_ogg, input->options.auto_ogg);
		} else if (input->options.type == INPUT_TYPE_BNK) {
			BRRLOG_MESSAGETNP(gbrrlog_level_last, BNK_FORMAT, "%-*s",
			    numbers->path_maximum_length, BRRTIL_NULSTR((char *)input->path.opaque));
			extract_bnk(numbers, input->options.dry_run, input->path.opaque,
			    input->options.inplace_revorb, input->options.auto_revorb,
			    input->options.inplace_ogg, input->options.auto_ogg,
			    input->options.bank_recurse);
		}
	}

	brrstg_delete(&directory);
	brrstg_delete(&base_name);
	brrstg_delete(&extension);
	return err;
}
