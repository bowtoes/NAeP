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

#include "NeArg.h"

static const char *const usg =
"Usage: NAeP [[OPTIONS] FILE]..."
"\nOptions:"
"\n    Options take affect on all files following and can be toggled."
"\n        -h, -help, -v        Print this help."
"\n    File Type Specification:"
"\n        -a, -auto, -detect   Autodetect filetype from header or extension"
"\n        -w, -wem, -weem      File(s) are weems to be converted to ogg."
"\n        -W, -wsp, -wisp      File(s) are wisps to have their weems extracted."
"\n        -b, -bnk, -bank      File(s) are banks to extract all referenced weems."
"\n        -o, -ogg             File(s) are ogg files to be revorbed."
"\n    Processing options:"
"\n        -R, -recurse         Search passed bank files for all referenced weems."
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
;
static void BRRCALL
printhelp()
{
	BRRLOG_NOR("NAeP - NieR: Automata extraction Protocol");
	BRRLOG_NOR("Compiled on "__DATE__", " __TIME__"\n");
	fprintf(stdout, "%s\n", usg);
	exit(0);
}

/* How work:
 * First, process all cmd arguments and build list of files, each with different,
 * specific options. Then go through the list and yada yada, do the extractions/conversions.
 * */
int main(int argc, char **argv)
{
	static const NeArgOptionsT default_options = {
		.logEnabled=1,
		.logPriority=brrlog_priority_debug,
		.logColorEnabled=1
	};
	static const brrct max_args_to_process = 1024;

	NeArgOptionsT opt = default_options;
	NeArgArrayT args = {0};
	brrsz max_arg_length = 0;
	char *arg = NULL;
	brrby rst = 0;

	brrlog_setlogmax(0);
	gbrrlogctl.style_enabled = 1;
	gbrrlogctl.flush_enabled = 1;

	if (argc == 1)
		printhelp();
	arg = argv[1];

	/* Parse arguments */
	for (int i = 1; i < argc; ++i, arg = argv[i]) {
		NeArgT narg = {0}, *t;
		if (-1 != brrstg_cstr_compare(arg, 0, "-h", "-help", "--help", "-v", "-version", "--version", NULL)) {
			printhelp();
		} else if (-1 != brrstg_cstr_compare(arg, 1, "-a", "-auto", "-detect", NULL)) {
			opt.weem = opt.wisp = opt.bank = opt.oggs = 0; continue;
		} else if (-1 != brrstg_cstr_compare(arg, 1, "-w", "-wem", "-weem", NULL)) {
			opt.weem = 1; opt.oggs = opt.wisp = opt.bank = 0; continue;
		} else if (-1 != brrstg_cstr_compare(arg, 1, "-W", "-wsp", "-wisp", NULL)) {
			opt.wisp = 1; opt.weem = opt.oggs = opt.bank = 0; continue;
		} else if (-1 != brrstg_cstr_compare(arg, 1, "-b", "-bnk", "-bank", NULL)) {
			opt.bank = 1; opt.weem = opt.wisp = opt.oggs = 0; continue;
		} else if (-1 != brrstg_cstr_compare(arg, 1, "-o", "-ogg", NULL)) {
			opt.oggs  = 1; opt.weem = opt.wisp = opt.bank = 0; continue;
		} else if (-1 != brrstg_cstr_compare(arg, 1, "-R", "-recurse", NULL)) {
			BRRTIL_TOGGLE(opt.recurseBanks); continue;
		} else if (-1 != brrstg_cstr_compare(arg, 1, "-O", "-weem2ogg", NULL)) {
			BRRTIL_TOGGLE(opt.autoOgg); continue;
		} else if (-1 != brrstg_cstr_compare(arg, 1, "-oi", "-ogg-inplace", NULL)) {
			BRRTIL_TOGGLE(opt.oggInplace); continue;
		} else if (-1 != brrstg_cstr_compare(arg, 1, "-r", "-revorb", NULL)) {
			BRRTIL_TOGGLE(opt.autoRevorb); continue;
		} else if (-1 != brrstg_cstr_compare(arg, 1, "-ri", "-rvb-inplace", NULL)) {
			BRRTIL_TOGGLE(opt.revorbInplace); continue;
		} else if (-1 != brrstg_cstr_compare(arg, 1, "-d", "-debug", NULL)) {
			BRRTIL_TOGGLE(opt.logDebug); continue;
		} else if (-1 != brrstg_cstr_compare(arg, 1, "-c", "-color", NULL)) {
			BRRTIL_TOGGLE(opt.logColorEnabled); continue;
		} else if (-1 != brrstg_cstr_compare(arg, 1, "-q", "-quiet", NULL)) {
			opt.logPriority = opt.logPriority - 1 < 0 ? 0 : opt.logPriority - 1; continue;
		} else if (-1 != brrstg_cstr_compare(arg, 1, "-Q", "-qq", "-too-quiet", NULL)) {
			BRRTIL_TOGGLE(opt.logEnabled); continue;
		} else if (-1 != brrstg_cstr_compare(arg, 1, "-n", "-dry", "-dryrun", NULL)) {
			BRRTIL_TOGGLE(opt.dryRun); continue;
		} else if (-1 != brrstg_cstr_compare(arg, 1, "-reset", NULL)) {
			BRRTIL_TOGGLE(rst); continue;
		} else if ((t = NeFindArg(args, arg))) {
			t->options = opt; opt = default_options; continue;
		}

		if (brrstg_new(&narg.argument, arg, -1)) {
			/* TODO error */
		}
		narg.options = opt;

		if (narg.argument.length > max_arg_length)
			max_arg_length = narg.argument.length;
		if (brrlib_alloc((void **)&args.args, (args.arg_count+1) * sizeof(NeArgT), 0)) {
			BRRDEBUG_TRACE("Could not allocate enough space for parsing %zu arguments", args.arg_count+1);
		}
		args.args[args.arg_count++] = narg;
		if (rst)
			opt = default_options;
	}
	args.arg_digit = brrlib_ndigits(args.arg_count, 0, 10);

	/* Process files */
	if (args.arg_count) {
		NeArgT *arg = &args.args[0];
		brrpath_stat_resultT st;
		for (int i = 0; i < args.arg_count; ++i, arg = &args.args[i]) {
			gbrrlogctl.debug_enabled = arg->options.logDebug;
			gbrrlogctl.style_enabled = arg->options.logColorEnabled;
			brrlog_setmaxpriority(arg->options.logEnabled?arg->options.logPriority:brrlog_priority_none);
			if (brrpath_stat(&arg->argument, &st)) {
				BRRLOG_ERR("Failed to stat '%s': %s", BRRTIL_NULSTR((char *)arg->argument.opaque), strerror(errno));
				brrstg_delete(&arg->argument);
				continue;
			} else if (!st.exists || st.type != brrpath_type_file) {
				BRRLOG_WARN("Cannot parse ");
				BRRLOG_MESSAGESNP(gbrrlog_type_last.level, brrlog_color_cyan, -1, brrlog_style_bold, -1, "%s", BRRTIL_NULSTR((char *)arg->argument.opaque));
				BRRLOG_WARNP(" : ");
				if (!st.exists)
					BRRLOG_MESSAGESP(gbrrlog_type_last.level, brrlog_color_red, -1, brrlog_style_bold, -1, "File does not exist");
				else if (st.type != brrpath_type_file)
					BRRLOG_MESSAGESP(gbrrlog_type_last.level, brrlog_color_magenta, -1, brrlog_style_bold, -1, "File is not regular");
				brrstg_delete(&arg->argument);
				continue;
			} else {
				BRRLOG_MESSAGESN(gbrrlog_type_debug.level, brrlog_color_green, gbrrlog_type_debug.format.foreground, brrlog_style_bold, -1, "TODO: ");
				BRRLOG_DEBUGP(" Check if can open file and open");
			}
#if 0
			} else if (!brrbuffer_from_file(BRRTIL_NULSTR((char *)arg->argument.opaque), NeFileModeReadWrite) != NeArgErr_None) {
				BRRLOG_ERRN("Could not open ");
				BRRLOG_MESSAGE_STNP(brrlog_format_last.level, brrlog_color_cyan, -1, brrlog_style_bold, "%s", BRRTIL_NULSTR((char *)arg->argument.opaque));
				BRRLOG_ERRP(" for parsing.");
				brrstg_delete(&arg->argument);
				continue;
			}
#endif
			BRRLOG_NORN("Parsing ");
			BRRLOG_MESSAGESNP(gbrrlog_type_last.level, brrlog_color_magenta, -1, -1, -1, "%*i / %*i ", args.arg_digit, i + 1, args.arg_digit, args.arg_count);
			if (arg->options.logDebug)
				NePrintArg(*arg, arg->options.logEnabled, max_arg_length);
			if ((arg->options.oggs | arg->options.weem | arg->options.wisp | arg->options.bank) == 0)
				NeDetectType(arg, &st, BRRTIL_NULSTR((char *)arg->argument.opaque));
			if (arg->options.oggs) {
				BRRLOG_MESSAGESP(gbrrlog_type_last.level, brrlog_color_blue, -1, -1, -1, "%-*s", max_arg_length, BRRTIL_NULSTR((char *)arg->argument.opaque));
				NeRevorbOgg(arg, &st, BRRTIL_NULSTR((char *)arg->argument.opaque));
			} else if (arg->options.weem) {
				BRRLOG_MESSAGESP(gbrrlog_type_last.level, brrlog_color_green, -1, -1, -1, "%-*s", max_arg_length, BRRTIL_NULSTR((char *)arg->argument.opaque));
				NeConvertWeem(arg, &st, BRRTIL_NULSTR((char *)arg->argument.opaque));
			} else if (arg->options.wisp) {
				BRRLOG_MESSAGESP(gbrrlog_type_last.level, brrlog_color_yellow, -1, -1, -1, "%-*s", max_arg_length, BRRTIL_NULSTR((char *)arg->argument.opaque));
				NeExtractWisp(arg, &st, BRRTIL_NULSTR((char *)arg->argument.opaque));
			} else if (arg->options.bank) {
				BRRLOG_MESSAGESP(gbrrlog_type_last.level, brrlog_color_red, -1, -1, -1, "%-*s", max_arg_length, BRRTIL_NULSTR((char *)arg->argument.opaque));
				NeExtractBank(arg, &st, BRRTIL_NULSTR((char *)arg->argument.opaque));
			} else {
				BRRLOG_ERRN("Could not determine filetype for ");
				BRRLOG_MESSAGESNP(gbrrlog_type_last.level, brrlog_color_cyan, -1, -1, -1, "%s", BRRTIL_NULSTR((char *)arg->argument.opaque));
				BRRLOG_ERRP(", cannot parse");
			}
			BRRLOG_MESSAGESN(gbrrlog_type_debug.level, brrlog_color_green, gbrrlog_type_debug.format.foreground, brrlog_style_bold, -1, "TODO: ");
			BRRLOG_DEBUGP(" Close file");
			/*
			if (NeFileClose(&afile) != NeERGNONE) {
				BRRDEBUG_TRACE("AAA");
			}
			*/
		}
	} else {
		brrlog_setmaxpriority(opt.logDebug?brrlog_priority_debug:opt.logPriority);
		BRRLOG_ERR("No files passed");
	}
	brrlib_alloc((void **)&args.args, 0, 0);

	return 0;
}

#if 0
static int extractwisp(struct NeArg a, struct NeWeem **dst) {
	struct NeWeem *d = *dst;
	struct NeWisp wsp = {0};
	struct NeStr wispPath, tempPath = {0};
	brrsz dgt = 0;
	int wemcount = 0, err = 0;

	if (!NeWispOpen(&wsp, a.arg)) {
		BRRLOG_ERR("Failed to open %s for wisp extraction");
		dst = NULL;
		return 0;

	}
	wispPath = wsp.file.ppp;
	NeStrSlice(&tempPath, wispPath, 0, NeStrRindex(wispPath, NeStrShallow(".", 1), wispPath.length));

	d = NeSafeAlloc(d, wsp.wemCount * sizeof(*d), 1);
	for (int i = 0; i < wsp.wemCount; ++i) {
		struct NeWem wem = wsp.wems[i];
		struct NeWeem w = {0};
		brrsz wrt = 0;

		NeStrPrint(&w.outpath, 0, NeMAXPATH - 4, "%s_%0*u", tempPath.cstr, dgt, i);
		NeStrPrint(&w.outpath, w.outpath.length, NeMAXPATH, "%s", ".wem");
		w.size = wem.size;

		while (wrt < wem.size + 8) {
			brrsz rd = 0;
			w.data = NeSafeAlloc(w.data, wrt + NeBLOCKSIZE, 0);
			rd = NeFileSegment(&wsp.file, w.data + wrt, NeBLOCKSIZE, wem.offset + wrt, wem.offset + wem.size + 8);
			if (rd == -1) {
				err = 1;
				break;
			}
			wrt += rd;
		}
		wemcount++;
	}
	NeWispClose(&wsp);
	return wemcount;
}
#endif
