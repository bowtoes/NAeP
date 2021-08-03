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

static void BRRCALL
print_help()
{
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
	"\n        -q, -quiet           Suppress one additional level of non-critical."
	"\n        +q, +quiet           Show one additional level non-critical output."
	"\n        -Q, -qq, -too-quiet  Suppress all output, including anything critical."
	"\n        -n, -dry, -dryrun    Don't actually do anything, just log what would happen."
	"\n        -reset               Argument options reset to default after each file passed."
	;
	fprintf(stdout, "NAeP - NieR: Automata extraction Protocol\n");
	fprintf(stdout, "Compiled on "__DATE__", " __TIME__"\n");
	fprintf(stdout, "%s\n", help);
	exit(0);
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
	static const global_optionsT default_global = {
		.should_reset = 0,
		.log_style_enabled = 1,
	};
	static numbersT numbers = {0};
	global_optionsT global_options = default_global;
	input_optionsT current_options = default_options;
	inputT *inputs = NULL;

	if (argc == 1) {
		print_help();
	} else if (brrlog_setlogmax(0)) {
		fprintf(stderr, "Failed to initialize logging output : %s", strerror(errno));
		return 1;
	}
	gbrrlogctl.flush_enabled = 1;
	gbrrlog_level_critical.prefix = "[CRAZY] ";
	gbrrlog_level_error.prefix    = "[ERROR] ";
	gbrrlog_level_warning.prefix  = "[CAUTION] ";
	gbrrlog_level_debug.prefix    = "[DEBUG] ";
	gbrrlog_format_debug = BRRLOG_FORMAT_FORE(brrlog_color_green);

	/* NOTE PARSE COMMAND-LINE ARGUMENTS */
	for (int i = 1; i < argc; ++i) {
		char *arg = argv[i];
		inputT next = {0};
		if (parse_argument(print_help, arg, &global_options, &current_options, inputs, numbers.paths_count, &default_options))
			continue;

		gbrrlogctl.style_enabled = global_options.log_style_enabled;
		next.options = current_options;
		if (brrstg_new(&next.path, arg, -1))
			BRRDEBUG_TRACE("Failed to initialize argument string %d '%s' : %s", i, arg, strerror(errno));
		if (brrlib_alloc((void **)&inputs, (numbers.paths_count + 1) * sizeof(inputT), 0))
			BRRDEBUG_TRACE("Failed to allocate space for next input %d '%s' : %s", i, arg, strerror(errno));
		inputs[numbers.paths_count++] = next;

		{
			brrsz tmp = 0;
			if ((tmp = strlen(arg)) > numbers.path_maximum_length)
				numbers.path_maximum_length = tmp;
		}

		if (global_options.should_reset) {
			current_options = default_options;
			global_options = default_global;
		}
	}

	/* NOTE PROCESS FILES */
	if (!numbers.paths_count) {
		brrlog_setmaxpriority(current_options.log_debug?brrlog_priority_debug:current_options.log_priority);
		BRRLOG_ERR("No files passed");
		return 1;
	}
	for (brrsz i = 0; i < numbers.paths_count; ++i) {
		inputT *const input = &inputs[i];
		brrpath_stat_resultT path_stat = {0};
		if (global_options.log_style_enabled) {
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
			process_input(input, &numbers, &path_stat, i);
		}
		input_delete(input);
	}
	brrlib_alloc((void **)&inputs, 0, 0);
}
