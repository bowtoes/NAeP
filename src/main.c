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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

#include <brrtools/brrplatform.h>
#include <brrtools/brrlib.h>
#include <brrtools/brrlog.h>
#include <brrtools/brrmem.h>
#include <brrtools/brrstg.h>
#include <brrtools/brrtil.h>
#include <brrtools/brrpath.h>

#include "common.h"
#include "process_files.h"

/*
 *
 * TODO
 * Right now, all codebooks passed in and the default are loaded before any
 * other file processing is done; so if any passed in codebooks aren't found
 * when you run the program, it will quit before processing anything.
 *
 * It should do a lazy init of the codebooks instead; if the relevant library
 * isn't found, error that input and continue.
 *
 * Also, only a naive strcmp is done when checking for duplicate library files,
 * so one could pass 'library.cbl' and './library.cbl' and that one library
 * would be allocated twice; this'll be something for brrpath to help with
 * eventually.
 *
 * */

static void BRRCALL
print_help()
{
	static const char *const help =
	"Usage: NAeP [[OPTION ...] FILE ...] ..."
	"\nOptions take affect on all files following and can be toggled."
	"\nOptions:"
	"\n        -h, -help, -v . . . . . . . . . . . .Print this help."
	"\n    File Type Specification:"
	"\n        -a, -auto, -detect. . . . . . . . . .Autodetect filetype from file header or extension."
	"\n        -w, -wem, -weem . . . . . . . . . . .File(s) are WEMs to be converted to OGG."
	"\n        -W, -wsp, -wisp . . . . . . . . . . .File(s) are wisps to have their WEMs extracted."
	"\n        -b, -bnk, -bank . . . . . . . . . . .File(s) are banks to extract all referenced WEMs."
	"\n        -o, -ogg. . . . . . . . . . . . . . .File(s) are OGG files to be regranularizeed."
	"\n    OGG Processing Options:"
	"\n        -ri, -rgrn-inplace, -rvb-inplace. . .Oggs are regranularized in-place."
	"\n    WSP Processing Options:"
	"\n        -O, -wem2ogg. . . . . . . . . . . . .Convert extracted WEMs to OGGs."
	"\n    WEM/BNK Processing Options:"
	"\n        -R, -recurse-bank . . . . . . . . . .Search passed bank files for all referenced WEMs."
	"\n        -oi, -ogg-inplace . . . . . . . . . .All WEM-to-OGG conversion is done in-place;"
	"\n                                             WEMs are replaced with their converted OGGs."
	"\n    Miscellaneous options:"
	"\n        --. . . . . . . . . . . . . . . . . .The following argument is a file path, not an option."
	"\n        -!. . . . . . . . . . . . . . . . . .All following arguments are file paths, not options."
	"\n        -d, -debug. . . . . . . . . . . . . .Enable debug output, irrespective of quiet settings."
	"\n        -c, -color. . . . . . . . . . . . . .Toggle color logging."
	"\n        -q, -quiet. . . . . . . . . . . . . .Suppress one additional level of non-critical."
	"\n        +q, +quiet. . . . . . . . . . . . . .Show one additional level non-critical output."
	"\n        -Q, -qq, -too-quiet . . . . . . . . .Suppress all output, including anything critical."
	"\n        -n, -dry, -dry-run. . . . . . . . . .Don't actually do anything, just log what would happen."
	"\n        -reset. . . . . . . . . . . . . . . .Argument options reset to default after each file passed."
	"\n        -cbl, -codebook-library . . . . . . .Which library binary to use for the following WEMs/BNKs."
	;
	fprintf(stdout, "NAeP - NieR: Automata extraction Protocol\n");
	fprintf(stdout, "Compiled on "__DATE__", " __TIME__"\n");
	fprintf(stdout, "%s\n", help);
	exit(0);
}

static int BRRCALL
i_init_brrlog(void)
{
	if (brrlog_setlogmax(0)) {
		return 1;
	}
	gbrrlogctl.flush_enabled = 1;
	gbrrlogctl.flush_always = 1;
	gbrrlog_level_critical.prefix = "[CRAZY] ";
	gbrrlog_level_error.prefix    = "[ERROR] ";
	gbrrlog_level_warning.prefix  = "[CAUTION] ";
	gbrrlog_level_debug.prefix    = "[DEBUG] ";
	gbrrlog_format_debug = BRRLOG_FORMAT_FORE(brrlog_color_green);

	return 0;
}
static processed_inputT *BRRCALL
i_find_input(const char *const arg, const processed_inputT *const inputs, brrsz input_count)
{
	for (brrsz i = 0; i < input_count; ++i) {
		if (0 == strcmp(inputs[i].path.opaque, arg))
			return (processed_inputT *)&inputs[i];
	}
	return NULL;
}

static int BRRCALL
i_determine_input_type(processed_inputT *const input, const char *const extension)
{
	int err = 0;
	fourccT input_fcc = {0};
	FILE *fp = NULL;
	if (!(fp = fopen((char *)input->path.opaque, "rb"))) {
		return I_IO_ERROR;
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
				err = I_UNRECOGNIZED_DATA;
			}
			if (!err) {
				BRRLOG_WARN("File is very short ");
			}
			return err;
		} else {
			err = I_IO_ERROR;
		}
	}
	fclose(fp);
	if (!err) {
		BRRLOG_DEBUGNP("FCC %08X = %02X %02X %02X %02X ", input_fcc, FCC_GET_BYTES(input_fcc));
		if (input_fcc.integer == goggfcc.integer) {
			input->options.type = INPUT_TYPE_OGG;
		} else if (input_fcc.integer == gwemfcc.integer) {
			if (!brrstg_cstr_compare(extension, 0, "wem", NULL))
				input->options.type = INPUT_TYPE_WEM;
			else if (!brrstg_cstr_compare(extension, 0, "wsp", NULL))
				input->options.type = INPUT_TYPE_WSP;
			else
				err = I_UNRECOGNIZED_DATA;
		} else if (input_fcc.integer == gbnkfcc.integer) {
			input->options.type = INPUT_TYPE_BNK;
		} else {
			err = I_UNRECOGNIZED_DATA;
		}
	}
	return err;
}
static int BRRCALL
i_parse_argument(void (*const print_help)(void),
    const char *const arg, global_optionsT *const global, input_optionsT *const options)
{
#define IF_CHECK_ARG(_cse_, ...) if (-1 != brrstg_cstr_compare(arg, _cse_, __VA_ARGS__, NULL))
	if (arg[0] == 0) { /* Argument is of 0 length, very bad! */
		return 1;
	} else if (global->always_file) {
		return 0;
	} else if (global->next_is_file) {
		global->next_is_file = 0;
		return 0;
	} else if (global->next_is_cbl) {
		return 0;
	} else IF_CHECK_ARG(0, "-h", "-help", "--help", "-v", "-version", "--version") {
		print_help();
	} else IF_CHECK_ARG(1, "-a", "-auto", "-detect") {
		options->type = INPUT_TYPE_UNK;
		return 1;
	} else IF_CHECK_ARG(1, "-o", "-ogg") {
		options->type = INPUT_TYPE_OGG;
		return 1;
	} else IF_CHECK_ARG(1, "-w", "-wem", "-weem") {
		options->type = INPUT_TYPE_WEM;
		return 1;
	} else IF_CHECK_ARG(1, "-W", "-wsp", "-wisp") {
		options->type = INPUT_TYPE_WSP;
		return 1;
	} else IF_CHECK_ARG(1, "-b", "-bnk", "-bank") {
		options->type = INPUT_TYPE_BNK;
		return 1;
	} else IF_CHECK_ARG(1, "-R", "-recurse-bank") {
		BRRTIL_TOGGLE(options->bank_recurse);
		return 1;
	} else IF_CHECK_ARG(1, "-O", "-wem2ogg") {
		BRRTIL_TOGGLE(options->auto_ogg);
		return 1;
	} else IF_CHECK_ARG(1, "-oi", "-ogg-inplace") {
		BRRTIL_TOGGLE(options->inplace_ogg);
		return 1;
	} else IF_CHECK_ARG(1, "-ri", "-rgrn-inplace", "-rvb-inplace") {
		BRRTIL_TOGGLE(options->inplace_regranularize);
		return 1;
	} else IF_CHECK_ARG(1, "-Q", "-qq", "-too-quiet") {
		BRRTIL_TOGGLE(options->log_enabled);
		return 1;
	} else IF_CHECK_ARG(1, "-c", "-color") {
		BRRTIL_TOGGLE(options->log_color_enabled);
		return 1;
	} else IF_CHECK_ARG(1, "-C", "-global-color") {
		BRRTIL_TOGGLE(global->log_style_enabled);
		return 1;
	} else IF_CHECK_ARG(1, "--") {
		global->next_is_file = 1;
		return 1;
	} else IF_CHECK_ARG(1, "-!") {
		global->always_file = 1;
		return 1;
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
	} else IF_CHECK_ARG(1, "-n", "-dry", "-dry-run") {
		BRRTIL_TOGGLE(options->dry_run);
		return 1;
	} else IF_CHECK_ARG(1, "-reset") {
		BRRTIL_TOGGLE(global->should_reset);
		return 1;
	} else IF_CHECK_ARG(1, "-cbl", "-codebook-library") {
		global->next_is_cbl = 1;
		return 1;
	}
	return 0;
#undef IF_CHECK_ARG
}
static brrsz BRRCALL
i_find_library(const char *const test_path, const input_libraryT *const libraries, brrsz library_count)
{
	brrsz idx = 0;
	for (;idx < library_count; ++idx) {
		if (0 == strcmp(libraries[idx].library_path.opaque, test_path))
			break;
	}
	return idx;
}
static int BRRCALL
i_add_library(input_libraryT **const libraries, numbersT *const nums, const char *const arg)
{
	input_libraryT next = {0};
	if (brrstg_new(&next.library_path, arg, -1)) {
		return 1;
	} else if (brrlib_alloc((void **)libraries, (nums->n_libraries + 1) * sizeof(input_libraryT), 0)) {
		brrstg_delete(&next.library_path);
		return 1;
	} else {
		/* Determine whether the library is the old type */
		brrsz dot = brrmem_previous(next.library_path.opaque, next.library_path.length,
		    '.', next.library_path.length - 1);
		if (dot < next.library_path.length) {
			char extension[6] = "";
			snprintf(extension, 6, "%s", (char *)next.library_path.opaque + dot);
			if (0 == cstr_compare(extension, "ocbl", 6, 0))
				next.old = 1;
		}
	}
	(*libraries)[nums->n_libraries++] = next;
	return 0;
}
static int
i_add_input(processed_inputT **const inputs, numbersT *const nums, input_optionsT *current, const char *const arg)
{
	processed_inputT next = {.options = *current};
	if (brrstg_new(&next.path, arg, -1)) {
		return 1;
	} else if (brrlib_alloc((void **)inputs, (nums->n_inputs + 1) * sizeof(processed_inputT), 0)) {
		brrstg_delete(&next.path);
		return 1;
	} else {
		(*inputs)[nums->n_inputs++] = next;
		if (next.path.length > nums->input_path_max_length)
			nums->input_path_max_length = next.path.length;
	}
	return 0;
}
static int BRRCALL
i_take_arguments(processed_inputT **const inputs, input_libraryT **const libraries, numbersT *const nums,
    input_optionsT default_options, input_optionsT *const current, global_optionsT *const global,
	int argc, char **argv)
{
	for (brrsz i = 0; i < argc; ++i) {
		char *arg = argv[i];
		processed_inputT *tmp_arg = NULL;
		if (i_parse_argument(print_help, arg, global, current)) {
			continue;
		} else if (global->next_is_cbl) {
			current->library_index = i_find_library(arg, *libraries, nums->n_libraries);
			if (current->library_index == nums->n_libraries) { /* Need to append library */
				if (i_add_library(libraries, nums, arg)) {
					BRRLOG_CRITICAL("Failed to add library '%s' to list : %s", arg, strerror(errno));
					continue;
				}
			}
			global->next_is_cbl = 0;
			continue;
		}
		if ((tmp_arg = i_find_input(arg, *inputs, nums->n_inputs))) {
			tmp_arg->options = *current;
		} else {
			gbrrlogctl.style_enabled = global->log_style_enabled;
			if (i_add_input(inputs, nums, current, arg)) {
				BRRLOG_CRITICAL("Failed to add input '%s' to list : %s", arg, strerror(errno));
				continue;
			}
		}
		if (global->should_reset)
			*current = default_options;
	}
	return 0;
}
static void BRRCALL
i_clear_inputs(processed_inputT **const inputs, brrsz n_inputs)
{
	if (*inputs) {
		for (brrsz i = 0; i < n_inputs; ++i) {
			processed_input_clear(&(*inputs)[i]);
		}
		free(*inputs);
		*inputs = NULL;
	}
}
static void BRRCALL
i_clear_libraries(input_libraryT **const libraries, brrsz n_libraries)
{
	if (*libraries) {
		for (brrsz i = 0; i < n_libraries; ++i) {
			input_library_clear(&(*libraries)[i]);
		}
		free(*libraries);
		*libraries = NULL;
	}
}

static int BRRCALL
i_process_input(processed_inputT *const input, input_libraryT *const libraries,
    numbersT *const numbers, brrsz posterity_index)
{
	int err = 0;
	brrsz input_count_digits = brrlib_ndigits(numbers->n_inputs, 0, 10);
	brrstgT directory = {0}, base_name = {0}, extension = {0};
	/* TODO Check if can open file */
	if (brrpath_split(&directory, &base_name, &extension, input->path.opaque)) {
		return -1;
	}
	BRRLOG_NORN("Parsing");
	BRRLOG_FORENP(INFO_COLOR, " %*i / %*i ",
	    input_count_digits, posterity_index + 1, input_count_digits, numbers->n_inputs);
#if !defined(NeDEBUG)
	if (input->options.log_debug) {
#endif
		processed_input_print(input, brrlog_priority_debug, 0, numbers->input_path_max_length);
#if !defined(NeDEBUG)
	}
#endif
	BRRLOG_NORNP(" "); /* Reset last log format and level */
	if (input->options.type == INPUT_TYPE_UNK) {
		int err = 0;
		if ((err = i_determine_input_type(input, extension.opaque))) {
			BRRLOG_ERRN("Failed to determine filetype for ");
			BRRLOG_FORENP(PATH_COLOR, "%-*s", numbers->input_path_max_length, BRRTIL_NULSTR((char *)input->path.opaque));
			BRRLOG_ERRP(" : %s", i_strerr(errno));
		}
	}
	if (!err) {
		BRRLOG_NORNP(""); /* Reset last log format and level */
		if (input->options.type == INPUT_TYPE_OGG) {
			BRRLOG_MESSAGETNP(gbrrlog_level_last, OGG_FORMAT, "%-*s",
			    numbers->input_path_max_length, BRRTIL_NULSTR((char *)input->path.opaque));
			regranularize_ogg(numbers, input->options.dry_run, input->path.opaque,
			    input->options.inplace_regranularize);
		} else if (input->options.type == INPUT_TYPE_WEM) {
			BRRLOG_MESSAGETNP(gbrrlog_level_last, WEM_FORMAT, "%-*s",
			    numbers->input_path_max_length, BRRTIL_NULSTR((char *)input->path.opaque));
			convert_wem(numbers, input->options.dry_run, input->path.opaque,
			    input->options.inplace_ogg,
			    &libraries[input->options.library_index]);
		} else if (input->options.type == INPUT_TYPE_WSP) {
			BRRLOG_MESSAGETNP(gbrrlog_level_last, WSP_FORMAT, "%-*s",
			    numbers->input_path_max_length, BRRTIL_NULSTR((char *)input->path.opaque));
			extract_wsp(numbers, input->options.dry_run, input->path.opaque,
			    input->options.inplace_ogg, input->options.auto_ogg,
			    &libraries[input->options.library_index]);
		} else if (input->options.type == INPUT_TYPE_BNK) {
			BRRLOG_MESSAGETNP(gbrrlog_level_last, BNK_FORMAT, "%-*s",
			    numbers->input_path_max_length, BRRTIL_NULSTR((char *)input->path.opaque));
			extract_bnk(numbers, input->options.dry_run, input->path.opaque,
			    input->options.inplace_ogg, input->options.auto_ogg, input->options.bank_recurse,
			    &libraries[input->options.library_index]);
		}
	}

	brrstg_delete(&directory);
	brrstg_delete(&base_name);
	brrstg_delete(&extension);
	return err;
}
static int BRRCALL
i_process_inputs(processed_inputT *const inputs, input_libraryT *const libraries, numbersT *const numbers,
    global_optionsT *const global)
{
	for (brrsz i = 0; i < numbers->n_inputs; ++i) {
		processed_inputT *const input = &inputs[i];
		brrpath_stat_resultT path_stat = {0};
		if (global->log_style_enabled) {
			gbrrlogctl.style_enabled = input->options.log_color_enabled;
		}
#if defined(NeDEBUG)
		gbrrlogctl.debug_enabled = 1;
		brrlog_setmaxpriority(brrlog_priority_debug);
#else
		gbrrlogctl.debug_enabled = input->options.log_debug;
		brrlog_setmaxpriority(input->options.log_debug?brrlog_priority_debug:
		    input->options.log_enabled?input->options.log_priority:
			brrlog_priority_none);
#endif
		if (brrpath_stat(&path_stat, input->path.opaque)) {
			BRRLOG_ERR("Failed to stat path '%s' : %s", (char *)input->path.opaque, strerror(errno));
		} else if (!path_stat.exists) {
			BRRLOG_WARN("Cannot parse ");
			BRRLOG_FORENP(brrlog_color_cyan, "%s", BRRTIL_NULSTR((char *)input->path.opaque));
			BRRLOG_WARP(" : Path does not exist");
		} else if (path_stat.type != brrpath_type_file) {
			BRRLOG_WARN("Cannot parse ");
			BRRLOG_FORENP(brrlog_color_cyan, "%s", BRRTIL_NULSTR((char *)input->path.opaque));
			BRRLOG_WARP(" : Path is not a regular file");
		} else {
			i_process_input(input, &libraries[input->options.library_index], numbers, i);
		}
		processed_input_clear(input);
	}
	return 0;
}

int main(int argc, char **argv)
{
	static const input_optionsT default_options = {
		.log_enabled=1,
		.log_color_enabled=1,
		.log_priority=brrlog_priority_normal,
#if defined(NeDEBUG)
		.log_debug=1,
#endif
	};
	static global_optionsT global_options = {
		.should_reset = 0,
		.log_style_enabled = 1,
	};
	input_optionsT current_options = default_options;
	processed_inputT *inputs = NULL;
	input_libraryT *libraries = NULL;
	numbersT numbers = {0};
	int err = 0;

	if (argc == 1) {
		print_help();
	} else if (i_init_brrlog()) {
		fprintf(stderr, "Failed to initialize logging output : %s", strerror(errno));
		return 1;
	}

	if (i_take_arguments(&inputs, &libraries, &numbers,
	    default_options, &current_options, &global_options,
	    argc - 1, argv + 1)) {
		i_clear_inputs(&inputs, numbers.n_inputs);
		return 1;
	}
	if (!numbers.n_inputs) {
		brrlog_setmaxpriority(current_options.log_debug?brrlog_priority_debug:current_options.log_priority);
		BRRLOG_ERR("No files passed");
		return 1;
	}

	if (i_process_inputs(inputs, libraries, &numbers, &global_options)) {
		err = 1;
	}
	i_clear_inputs(&inputs, numbers.n_inputs);
	i_clear_libraries(&libraries, numbers.n_libraries);
	return err;
}
