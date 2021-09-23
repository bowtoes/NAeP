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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <brrtools/brrlib.h>
#include <brrtools/brrlog.h>
#include <brrtools/brrmem.h>

#include "common_input.h"
#include "common_lib.h"
#include "errors.h"
#include "process_files.h"
#include "riff.h"

/*
 * TODO
 * Only a naive strcmp is done when checking for duplicate files, so one could
 * pass the same file differently and that one file would be allocated and
 * processed twice; this'll be something for brrpath to help with eventually.
 * */

#define USAGE "Usage: NAeP [[OPTION ...] FILE ...] ..." \
"\nNAeP - NieR:Automated extraction Preceptv"NeVERSION"" \
"\nCompiled on "__DATE__", " __TIME__"\n"
#define HELP \
"Most options take affect on all files following and can be toggled." \
"\nSome options are global, and apply to the meta-process itself, marked with (g)." \
"\nOptions:" \
"\n        -h, -help, -v . . . . . . . . . . . .Print this help." \
"\n    File Type Specification:" \
"\n        -a, -auto, -detect  . . . . . . . .  Autodetect filetype from file header or extension." \
"\n        -w, -wem, -weem . . . . . . . . . .  File(s) are WEMs to be converted to OGG." \
"\n        -W, -wsp, -wisp . . . . . . . . . .  File(s) are wisps to have their WEMs extracted." \
"\n        -b, -bnk, -bank . . . . . . . . . .  File(s) are banks to extract all referenced WEMs." \
"\n        -o, -ogg  . . . . . . . . . . . . .  File(s) are OGG files to be regranularizeed." \
"\n    OGG Processing Options:" \
"\n        -ri, -rgrn-inplace, -rvb-inplace. .  Oggs are regranularized in-place." \
"\n    WEM Processing Options:" \
"\n        -oi, -ogg-inplace . . . . . . . . .  All WEM-to-OGG conversion is done in-place;" \
"\n                                             WEMs are replaced with their converted OGGs." \
"\n        -cbl, -codebook-library . . . . . .  The following is a codebook library to use for the following WEMs;" \
"\n                                             if none are specified for a given WEM, then it is assumed the" \
"\n                                             codebooks are inline." \
"\n        -inline . . . . . . . . . . . . . .  The following WEMs have inline codebooks." \
"\n        -stripped . . . . . . . . . . . . .  The vorbis headers of the WEM are stripped." \
"\n    WSP/BNK Processing Options:" \
"\n        -w2o, -wem2ogg. . . . . . . . . . .  Convert extracted WEMs to OGGs." \
"\n        -Rbk, -recurse-bank . . . . . . . .  Search passed bank files for all referenced WEMs." \
"\n    Miscellaneous options:" \
"\n        --  . . . . . . . . . . . . . . . .  The following argument is a file path, not an option." \
"\n        -!  . . . . . . . . . . . . . . . .  All following arguments are file paths, not options." \
"\n        -d, -debug  . . . . . . . . . . . .  Enable debug output, irrespective of quiet settings." \
"\n        -c, -color  . . . . . . . . . . . .  Toggle color logging." \
"\n        -C, -global-color (g) . . . . . . .  Toggle whether log styling is enabled at all." \
"\n        -r, -report-card (g)  . . . . . . .  Print a status report of all files processed after processing." \
"\n        +r, -full-report (g)  . . . . . . .  Print a more full report of all files processed." \
"\n        -q, -quiet  . . . . . . . . . . . .  Suppress one additional level of non-critical." \
"\n        +q, +quiet  . . . . . . . . . . . .  Show one additional level non-critical output." \
"\n        -Q, -qq, -too-quiet . . . . . . . .  Suppress all output, including anything critical." \
"\n        -n, -dry, -dry-run  . . . . . . . .  Don't actually do anything, just log what would happen." \
"\n        -reset (g)  . . . . . . . . . . . .  Argument options reset to default values after each file passed." \

static int BRRCALL
print_usage(void)
{
	fprintf(stdout, USAGE"\n""    -h, -help, -v . . . . . . . . . . . .Print help.""\n");
	exit(0);
	return 0;
}
static int BRRCALL
print_help(void)
{
	fprintf(stdout, USAGE"\n"HELP"\n");
	exit(0);
	return 0;
}
static int BRRCALL
print_numbers(const global_optionsT *const global, const numbersT *const numbers)
{
	brrsz input_count_digits = 1 + brrlib_ndigits(numbers->n_inputs, 0, 10);
	brrsz total_success =
	      numbers->oggs_regrained
	    + numbers->wems_converted
	    + numbers->wsps_processed
	    + numbers->bnks_processed;
	brrsz total_failure =
	      numbers->oggs_failed
	    + numbers->wems_failed
	    + numbers->wsps_failed
	    + numbers->bnks_failed;
	BRRLOG_NORN("Successfully processed a total of ");
	BRRLOG_FORENP(LOG_COLOR_INFO, "%*i / %*i",
	    input_count_digits, total_success, input_count_digits, numbers->n_inputs);
	BRRLOG_NORP(" inputs");
	if (global->full_report) {
		if (numbers->oggs_to_regrain) {
			BRRLOG_NORN("    ");
			BRRLOG_FORENP(LOG_COLOR_INFO, "%*i / %*i",
				input_count_digits, numbers->oggs_regrained, input_count_digits, numbers->oggs_to_regrain);
			BRRLOG_MESSAGETP(gbrrlog_level_last, LOG_FORMAT_OGG, " Regrained Oggs");
		}
		if (numbers->wems_to_convert) {
			BRRLOG_NORN("    ");
			BRRLOG_FORENP(LOG_COLOR_INFO, "%*i / %*i",
				input_count_digits, numbers->wems_converted, input_count_digits, numbers->wems_to_convert);
			BRRLOG_MESSAGETP(gbrrlog_level_last, LOG_FORMAT_WEM, " Converted WEMs");
		}
		if (numbers->wsps_to_process) {
			BRRLOG_NORN("    ");
			BRRLOG_FORENP(LOG_COLOR_INFO, "%*i / %*i",
				input_count_digits, numbers->wsps_processed, input_count_digits, numbers->wsps_to_process);
			BRRLOG_MESSAGETP(gbrrlog_level_last, LOG_FORMAT_WSP, " Processed WSPs");
		}
		if (numbers->bnks_to_process) {
			BRRLOG_NORN("    ");
			BRRLOG_FORENP(LOG_COLOR_INFO, "%*i / %*i",
				input_count_digits, numbers->bnks_processed, input_count_digits, numbers->bnks_to_process);
			BRRLOG_MESSAGETP(gbrrlog_level_last, LOG_FORMAT_BNK, " Processed BNKs");
		}
		if (numbers->wems_to_extract) {
			BRRLOG_NORN("    ");
			BRRLOG_FORENP(LOG_COLOR_INFO, "%*i / %*i",
				input_count_digits, numbers->wems_extracted, input_count_digits, numbers->wems_to_extract);
			BRRLOG_MESSAGETP(gbrrlog_level_last, LOG_FORMAT_WEM, " Extracted WEMs");
		}
		if (numbers->wems_to_convert_extract) {
			BRRLOG_NORN("    ");
			BRRLOG_FORENP(LOG_COLOR_INFO, "%*i / %*i",
				input_count_digits, numbers->wems_convert_extracted, input_count_digits, numbers->wems_to_convert_extract);
			BRRLOG_MESSAGETP(gbrrlog_level_last, LOG_FORMAT_OGG, " Auto-converted WEMs");
		}
	}
	return 0;
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
static inputT *BRRCALL
i_find_input(const char *const arg, const inputT *const inputs, brrsz input_count)
{
	for (brrsz i = 0; i < input_count; ++i) {
		if (0 == strcmp(inputs[i].path.opaque, arg))
			return (inputT *)&inputs[i];
	}
	return NULL;
}

static int BRRCALL
i_determine_input_type(inputT *const input, const char *const extension)
{
	static const brru4 oggcc = FCC_GET_INT("OggS");
	static const brru4 bnkcc = FCC_GET_INT("BKHD");

	int err = 0;
	fourccT input_fcc = {0};
	FILE *fp = NULL;
	if (!(fp = fopen((char *)input->path.opaque, "rb"))) {
		return I_IO_ERROR;
	} else if (4 != fread(&input_fcc.integer, 1, 4, fp)) {
		if (feof(fp)) {
			fclose(fp);
			if (!brrstg_cstr_compare(extension, 0, "ogg", NULL)) {
				input->options.type = input_type_ogg;
			} else if (!brrstg_cstr_compare(extension, 0, "wem", NULL)) {
				input->options.type = input_type_wem;
			} else if (!brrstg_cstr_compare(extension, 0, "wsp", NULL)) {
				input->options.type = input_type_wsp;
			} else if (!brrstg_cstr_compare(extension, 0, "bnk", NULL)) {
				input->options.type = input_type_bnk;
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
		if (input_fcc.integer == oggcc) {
			input->options.type = input_type_ogg;
		} else if (riff_cc_root(input_fcc.integer)) {
			if (!brrstg_cstr_compare(extension, 0, "wem", NULL))
				input->options.type = input_type_wem;
			else
				input->options.type = input_type_wsp; /* Default to wsp */
		} else if (input_fcc.integer == bnkcc) {
			input->options.type = input_type_bnk;
		} else {
			err = I_UNRECOGNIZED_DATA;
		}
	}
	return err;
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
			if (0 == lib_cstr_compare(extension, "ocbl", 6, 0))
				next.old = 1;
		}
	}
	(*libraries)[nums->n_libraries++] = next;
	return 0;
}
static int
i_add_input(inputT **const inputs, numbersT *const nums, input_optionsT *current, const char *const arg)
{
	inputT next = {.options = *current};
	if (brrstg_new(&next.path, arg, -1)) {
		return 1;
	} else if (brrlib_alloc((void **)inputs, (nums->n_inputs + 1) * sizeof(inputT), 0)) {
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
i_crement_log(input_optionsT *const options, int delta)
{
	if (delta <= 0) {
		if (options->log_priority > 0)
			options->log_priority--;
		if (options->log_priority == 0 && !options->log_debug)
			options->log_enabled = 0;
		return 1;
	} else {
		if (options->log_priority < brrlog_priority_count - 1)
			options->log_priority++;
		options->log_enabled = 1;
		return 1;
	}
}
static int BRRCALL
i_parse_argument(const char *const arg, global_optionsT *const global, input_optionsT *const options)
{
	/* TODO reorder the checks in the order of the help */
	if (arg[0] == 0) { /* Argument is of 0 length, very bad! */
		return 1;
	} else if (global->always_file) {
		return 0;
	} else if (global->next_is_file) {
		global->next_is_file = 0;
		return 0;
	} else if (global->next_is_cbl) {
		return 0;
	}
#define IF_CHECK_ARG(_cse_, ...) if (-1 != brrstg_cstr_compare(arg, _cse_, __VA_ARGS__, NULL))
#define CHECK_TOGGLE_ARG(_c_, _a_, ...) IF_CHECK_ARG((_c_), __VA_ARGS__) { (_a_) = !(_a_); return 1; }
#define CHECK_SET_ARG(_c_, _a_, _v_, ...) IF_CHECK_ARG((_c_), __VA_ARGS__) { (_a_) = (_v_); return 1; }
#define CHECK_RUN_ARG(_c_, _a_, ...) IF_CHECK_ARG((_c_), __VA_ARGS__) { _a_; }
	else CHECK_RUN_ARG(0, return print_help(), "-h", "-help", "--help", "-v", "-version", "--version")
	else CHECK_SET_ARG(1, options->type, input_type_auto, "-a", "-auto", "-detect")
	else CHECK_SET_ARG(1, options->type, input_type_ogg, "-a", "-ogg")
	else CHECK_SET_ARG(1, options->type, input_type_wem, "-w", "-wem", "-weem")
	else CHECK_SET_ARG(1, options->type, input_type_wsp, "-W", "-wsp", "-wisp")
	else CHECK_SET_ARG(1, options->type, input_type_bnk, "-b", "-bnk", "-bank")
	else CHECK_TOGGLE_ARG(1, options->bank_recurse, "-Rbnk", "-recurse_bank")
	else CHECK_TOGGLE_ARG(1, options->auto_ogg, "-w2o", "-wem2ogg")
	else CHECK_TOGGLE_ARG(1, options->inplace_ogg, "-oi", "-ogg-inplace")
	else CHECK_TOGGLE_ARG(1, options->inplace_regrain, "-ri", "-rgrn-inplace", "-rvb-inplace")
	else CHECK_SET_ARG(1, global->next_is_cbl, 1, "-cbl", "-codebook-library")
	else CHECK_SET_ARG(1, options->library_index, -1, "-inline")
	else CHECK_TOGGLE_ARG(1, options->stripped_headers, "-stripped")
	else CHECK_TOGGLE_ARG(1, options->log_enabled, "-Q", "-qq", "too-quiet")
	else CHECK_TOGGLE_ARG(1, options->log_color_enabled, "-c", "-color")
	else CHECK_TOGGLE_ARG(1, global->log_style_enabled, "-C", "-global-color")
	else CHECK_TOGGLE_ARG(1, global->report_card, "-r", "-report-card")
	else CHECK_TOGGLE_ARG(1, global->full_report, "+r", "-full-report")
	else CHECK_TOGGLE_ARG(1, global->next_is_file, "--")
	else CHECK_TOGGLE_ARG(1, global->always_file, "-!")
	else CHECK_TOGGLE_ARG(1, options->log_debug, "-d", "-debug")
	else CHECK_RUN_ARG(1, return i_crement_log(options, +1), "+q", "+quiet")
	else CHECK_RUN_ARG(1, return i_crement_log(options, -1), "-q", "-quiet")
	else CHECK_TOGGLE_ARG(1, options->dry_run, "-n", "-dry", "-dry-run")
	else CHECK_TOGGLE_ARG(1, global->should_reset, "-reset")
#undef IF_CHECK_ARG
#undef CHECK_TOGGLE_ARG
#undef CHECK_SET_ARG
#undef CHECK_RUN_ARG
	else return 0;
}
static int BRRCALL
i_take_arguments(inputT **const inputs, input_libraryT **const libraries, numbersT *const nums,
    input_optionsT default_options, input_optionsT *const current, global_optionsT *const global,
	int argc, char **argv)
{
	for (brrsz i = 0; i < argc; ++i) {
		char *arg = argv[i];
		inputT *tmp_arg = NULL;
		if (i_parse_argument(arg, global, current)) {
			continue;
		} else if (global->next_is_cbl) {
			current->library_index = i_find_library(arg, *libraries, nums->n_libraries);
			if (current->library_index == nums->n_libraries) {
				/* Library not found, add it */
				if (i_add_library(libraries, nums, arg)) {
					BRRLOG_CRITICAL("Failed to add library '%s' to list : %s", arg, strerror(errno));
				}
			}
			global->next_is_cbl = 0;
		} else if ((tmp_arg = i_find_input(arg, *inputs, nums->n_inputs))) {
			tmp_arg->options = *current;
		} else {
			gbrrlogctl.style_enabled = global->log_style_enabled;
			if (i_add_input(inputs, nums, current, arg)) {
				BRRLOG_CRITICAL("Failed to add input '%s' to list : %s", arg, strerror(errno));
			}
		}
		if (global->should_reset)
			*current = default_options;
	}
	return 0;
}
static void BRRCALL
i_clear_inputs(inputT **const inputs, brrsz n_inputs)
{
	if (*inputs) {
		for (brrsz i = 0; i < n_inputs; ++i) {
			input_clear(&(*inputs)[i]);
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
i_process_input(inputT *const input, input_libraryT *const libraries,
    numbersT *const numbers, brrsz posterity_index)
{
	int err = 0;
	brrsz input_count_digits = brrlib_ndigits(numbers->n_inputs, 0, 10);
	brrstgT directory = {0}, base_name = {0}, extension = {0};
	/* TODO Check if can open file */
	if (brrpath_split(&directory, &base_name, &extension, input->path.opaque)) {
		return -1;
	}
	BRRLOG_NORN("Parsing ");
	BRRLOG_FORENP(LOG_COLOR_INFO, "%*i / %*i",
	    input_count_digits, posterity_index + 1, input_count_digits, numbers->n_inputs);
	if (input->options.log_debug) {
		BRRLOG_NORNP(" ");
		input_print(input, numbers->input_path_max_length, brrlog_priority_debug, 0);
	}
	BRRLOG_NORNP(" "); /* Reset last log format and level */
	if (input->options.type == input_type_auto) {
		BRRLOG_MESSAGETNP(gbrrlog_level_last, LOG_FORMAT_AUT, "%-*s",
		    numbers->input_path_max_length, input->path.opaque);
		err = i_determine_input_type(input, extension.opaque);
	} else if (input->options.type == input_type_ogg) {
		BRRLOG_MESSAGETNP(gbrrlog_level_last, LOG_FORMAT_OGG, "%-*s",
		    numbers->input_path_max_length, input->path.opaque);
	} else if (input->options.type == input_type_wem) {
		BRRLOG_MESSAGETNP(gbrrlog_level_last, LOG_FORMAT_OGG, "%-*s",
		    numbers->input_path_max_length, input->path.opaque);
	} else if (input->options.type == input_type_wsp) {
		BRRLOG_MESSAGETNP(gbrrlog_level_last, LOG_FORMAT_OGG, "%-*s",
		    numbers->input_path_max_length, input->path.opaque);
	} else if (input->options.type == input_type_bnk) {
		BRRLOG_MESSAGETNP(gbrrlog_level_last, LOG_FORMAT_OGG, "%-*s",
		    numbers->input_path_max_length, input->path.opaque);
	}
	BRRLOG_NORNP(" "); /* Reset last log format and level */
	if (err) {
		BRRLOG_ERR("Failed to determine filetype : %s", lib_strerr(err));
	} else {
		input_libraryT *library = NULL;
		if (input->options.library_index != -1)
			library = &libraries[input->options.library_index];
		if (input->options.type == input_type_ogg) {
			regrain_ogg(numbers, input->path.opaque, input->path.length, &input->options);
		} else if (input->options.type == input_type_wem) {
			convert_wem(numbers, input->path.opaque, input->path.length, &input->options, library);
		} else if (input->options.type == input_type_wsp) {
			extract_wsp(numbers, input->path.opaque, input->path.length, &input->options, library);
		} else if (input->options.type == input_type_bnk) {
			extract_bnk(numbers, input->path.opaque, input->path.length, &input->options, library);
		}
	}

	brrstg_delete(&directory);
	brrstg_delete(&base_name);
	brrstg_delete(&extension);
	return err;
}
static int BRRCALL
i_process_inputs(inputT *const inputs, input_libraryT *const libraries, numbersT *const numbers,
    global_optionsT *const global)
{
	for (brrsz i = 0; i < numbers->n_inputs; ++i) {
		inputT *const input = &inputs[i];
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
			BRRLOG_FORENP(brrlog_color_cyan, "%s", input->path.opaque);
			BRRLOG_WARP(" : Path does not exist");
		} else if (path_stat.type != brrpath_type_file) {
			BRRLOG_WARN("Cannot parse ");
			BRRLOG_FORENP(brrlog_color_cyan, "%s", input->path.opaque);
			BRRLOG_WARP(" : Path is not a regular file");
		} else {
			i_process_input(input, &libraries[input->options.library_index], numbers, i);
		}
		input_clear(input);
	}
	return 0;
}

int main(int argc, char **argv)
{
	static const input_optionsT default_options = {
		.library_index = -1,
		.log_enabled = 1,
		.log_color_enabled = 1,
		.log_priority = brrlog_priority_normal,
#if defined(NeDEBUG)
		.log_debug = 1,
#endif
	};
	static global_optionsT global_options = {
		.should_reset = 0,
		.log_style_enabled = 1,
		.report_card = 1,
		.full_report = 1,
	};
	numbersT numbers = {0};
	input_optionsT current_options = default_options;
	input_libraryT *libraries = NULL;
	inputT *inputs = NULL;
	int err = 0;

	if (argc == 1) {
		print_usage();
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

	if (global_options.report_card || global_options.full_report)
		print_numbers(&global_options, &numbers);
	brrlog_deinit();
	return err;
}
