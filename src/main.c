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
#include <brrtools/brrstg.h>
#include <brrtools/brrtil.h>
#include <brrtools/brrpath.h>

#include "common.h"
#include "process.h"

static const char *const help =
"Usage: NAeP [[OPTION ...] FILE ...] ..."
"\nOptions take affect on all files following and can be toggled."
"\nOptions:"
"\n        -h, -help, -v        Print this help."
"\n    File Type Specification:"
"\n        -a, -auto, -detect   Autodetect filetype from header or extension"
"\n        -w, -wem, -weem      File(s) are weems to be converted to ogg."
"\n        -W, -wsp, -wisp      File(s) are wisps to have their weems extracted."
"\n        -b, -bnk, -bank      File(s) are banks to extract all referenced weems."
"\n        -o, -ogg             File(s) are ogg files to be revorbed."
"\n    Processing options:"
"\n        -R, -recurse-bank    Search passed bank files for all referenced weems."
"\n        -O, -weem2ogg        Convert extracted weems to oggs."
"\n        -oi, -ogg-inplace    All weem-to-ogg conversion is done in place;"
"\n                             weems are replaced with oggs."
"\n        -r, -revorb          All extracted/specified oggs are revorbed."
"\n        -ri, -rvb-inplace    Revorptions are done in place."
"\n    Miscellaneous options:"
"\n        -d, -debug           Enable debug output, irrespective of quiet settings."
"\n        -c, -color           Toggle color logging."
"\n        -q, -quiet           Suppress all non-critical output."
"\n        -Q, -qq, -too-quiet  Suppress all output."
"\n        -n, -dry, -dryrun    Don't actually do anything, just log what would happen."
"\n        -reset               Argument options reset to default after each file passed."
;
static void BRRCALL
print_help()
{
	fprintf(stdout, "NAeP - NieR: Automata extraction Protocol\n");
	fprintf(stdout, "Compiled on "__DATE__", " __TIME__"\n");
	fprintf(stdout, "%s\n", help);
	exit(0);
}

/* How work:
 * First, process all cmd arguments and build list of files, each with different,
 * specific options. Then go through the list and yada yada, do the extractions/conversions.
 * */
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
	input_optionsT current_options = default_options;
	int should_reset = 0;
	inputT *inputs = NULL;
	brrsz input_count = 0;
	brrsz input_count_digits = 0, max_input_length = 0;

	if (argc == 1) {
		print_help();
	} else if (brrlog_setlogmax(0)) {
		fprintf(stderr, "Failed to initialize logging output : %s", strerror(errno));
		return 1;
	}
	gbrrlogctl.flush_enabled = 1;

	/* PARSE COMMAND-LINE ARGUMENTS */
	for (int i = 1; i < argc; ++i) {
		inputT next = {0};
		char *arg = argv[i];
		brrsz n = 0;
		if (parse_argument(print_help, arg, &should_reset, &current_options, inputs, input_count, &default_options))
			continue;

		next.options = current_options;
		if (brrstg_new(&next.input, arg, -1))
			BRRDEBUG_TRACE("Failed to initialize argument string %d '%s' : %s", i, arg, strerror(errno));
		if (brrlib_alloc((void **)&inputs, (input_count + 1) * sizeof(inputT), 0))
			BRRDEBUG_TRACE("Failed to allocate space for next input %d '%s' : %s", i, arg, strerror(errno));
		inputs[input_count++] = next;

		if ((n = strlen(arg)) > max_input_length)
			max_input_length = n;

		if (should_reset) {
			current_options = default_options;
			should_reset = 0;
		}
	}

	/* PROCESS FILES */
	if (!input_count) {
		brrlog_setmaxpriority(current_options.log_debug?brrlog_priority_debug:current_options.log_priority);
		BRRLOG_ERR("No files passed");
		return 1;
	}
	input_count_digits = brrlib_ndigits(input_count, 0, 10);
	for (brrsz i = 0; i < input_count; ++i) {
		inputT *const input = &inputs[i];
		brrpath_infoT input_info = {0};
#if defined(NeDEBUG)
		gbrrlogctl.style_enabled = 1;
		gbrrlogctl.debug_enabled = 1;
		brrlog_setmaxpriority(brrlog_priority_debug);
#else
		gbrrlogctl.style_enabled = input->options.log_color_enabled;
		gbrrlogctl.debug_enabled = input->options.log_debug;
		brrlog_setmaxpriority(input->options.log_debug?brrlog_priority_debug:
		    input->options.log_enabled?input->options.log_priority:
			brrlog_priority_none);
#endif
		if (brrpath_info_new(&input_info, &input->input)) {
			BRRDEBUG_TRACE("Failed to initialize path info for '%s' : %s", BRRTIL_NULSTR((char *)input->input.opaque), strerror(errno));
			return 2;
		}
		if (!input_info.exists) {
			BRRLOG_WARN("Cannot parse ");
			BRRLOG_FORENP(brrlog_color_cyan, "%s", BRRTIL_NULSTR((char *)input->input.opaque));
			BRRLOG_WARP(" : Path does not exist");
		} else if (input_info.type != brrpath_type_file) {
			BRRLOG_WARN("Cannot parse ");
			BRRLOG_FORENP(brrlog_color_cyan, "%s", BRRTIL_NULSTR((char *)input->input.opaque));
			BRRLOG_WARP(" : Path is not a regular file");
		} else {
			/* TODO Check if can open file */
			BRRLOG_NORN("Parsing");
			BRRLOG_STYLENP(brrlog_color_magenta, -1, -1, " %*i / %*i", input_count_digits, i + 1, input_count_digits, input_count);
#if !defined(NeDEBUG)
			if (input->options.log_debug) {
#endif
				input_print(brrlog_priority_debug, 0, input, max_input_length);
#if !defined(NeDEBUG)
			}
#endif
			if (input->options.type == INPUT_TYPE_UNK) {
				int err = 0;
				if ((err = determine_type(input, &input_info))) {
					BRRLOG_ERRN("Failed to determine filetype for");
					BRRLOG_FONTNP(brrlog_color_cyan, -1, -1, -1, " %s", BRRTIL_NULSTR((char *)input->input.opaque));
					if (err == TYPE_ERR_INPUT)
						BRRLOG_ERRP(" : Could not open file for reading (%s)", strerror(errno));
					else if (err == TYPE_ERR_READ)
						BRRLOG_ERRP(" : Failed to read four-character-code of file : %s", strerror(errno));
					else
						BRRLOG_ERRP(" : File has no recognized type");
					continue;
				}
			}
			if (input->options.type == INPUT_TYPE_OGG) {
				BRRLOG_FORENP(brrlog_color_blue, " %-*s", max_input_length, BRRTIL_NULSTR((char *)input->input.opaque));
				if (input->options.dry_run) {
					BRRLOG_FORENP(brrlog_color_magenta, " Revorb OGG");
				} else {
					NeTODO("REVORG OGG");
				}
			} else if (input->options.type == INPUT_TYPE_WEM) {
				BRRLOG_FORENP(brrlog_color_green,  " %-*s", max_input_length, BRRTIL_NULSTR((char *)input->input.opaque));
				if (input->options.dry_run) {
					BRRLOG_FORENP(brrlog_color_magenta, " Convert WEM");
				} else {
					NeTODO("CONVERT WEM");
				}
			} else if (input->options.type == INPUT_TYPE_WSP) {
				BRRLOG_FORENP(brrlog_color_yellow, " %-*s", max_input_length, BRRTIL_NULSTR((char *)input->input.opaque));
				if (input->options.dry_run) {
					BRRLOG_FORENP(brrlog_color_magenta, " Extract WSP");
				} else {
					NeTODO("EXTRACT WSP");
				}
			} else if (input->options.type == INPUT_TYPE_BNK) {
				BRRLOG_FORENP(brrlog_color_red, " %-*s", max_input_length, BRRTIL_NULSTR((char *)input->input.opaque));
				if (input->options.dry_run) {
					BRRLOG_FORENP(brrlog_color_magenta, " Extract BNK");
				} else {
					NeTODO("EXTRACT BANK");
				}
			}
			BRRLOG_NORNP("\n");
		}
		brrpath_info_delete(&input_info);
		input_delete(input);
	}
	brrlib_alloc((void **)&inputs, 0, 0);
}
