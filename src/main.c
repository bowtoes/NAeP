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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <brrtools/brrdebug.h>
#include <brrtools/brrlog.h>

#include "common/NeLibrary.h"
#include "common/NeMisc.h"
#include "common/NeStr.h"
#include "wisp/NeWisp.h"

// TODO
// TODO
// TODO
// TODO
// TODO
// TODO
// TODO
// TODO NeArg, NeFile, NeWisp need a desperate rework/redesign/refactor all of it.
// TODO Also, the first file doesn't seem to take the arguments? Everything after does though?

#if 0
#define TEST(a, b) do { \
	printf("%s %s end with %s\n", a.cstr, NeStrEndswith(a, b)?"does":"does not", b.cstr); \
	printf("%s %s end with %s\n", b.cstr, NeStrEndswith(b, a)?"does":"does not", a.cstr); \
} while (0);
int main(int argc, char **argv) {
	struct NeStr a = NeStrShallow("woah.mp4.as", -1),
				 b = NeStrShallow(".mp4.as", -1),
				 c = NeStrShallow(".as", -1);
	TEST(a,b);
	TEST(a,c);
	TEST(b,c);
}
#else
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

static void printhelp()
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

#include "NeArg.h"
struct NeArgOpt def = {0};
int main(int argc, char **argv)
{
	struct NeArgOpt opt = {0};
	struct NeArgs args = {0};
	char *arg = argv[1];
	brrct maxargs = 1024;
	brrby rst = 0;

	brrlog_setlogmax(0);
	brrlogctl_styleon = true;
	brrlogctl_flushon = true;

	def.loglevel = brrlog_priority_debug;
	def.logcolor = 1;
	opt = def;

	if (argc == 1)
		printhelp();

	/* Parse arguments */
	for (int i = 1; i < argc; ++i, arg = argv[i]) {
		struct NeArg narg = {0}, *t;
		if (NeStrCmp(arg, 1, "-h", "-help", "--help", "-v", "-version", "--version", NULL)) {
			printhelp();
		} else if (NeStrCmp(arg, 0, "-a", "-auto", "-detect", NULL)) {
			opt.weem = opt.wisp = opt.bank = opt.oggs = 0; continue;
		} else if (NeStrCmp(arg, 0, "-w", "-wem", "-weem", NULL)) {
			opt.weem = 1; opt.oggs = opt.wisp = opt.bank = 0; continue;
		} else if (NeStrCmp(arg, 0, "-W", "-wsp", "-wisp", NULL)) {
			opt.wisp = 1; opt.weem = opt.oggs = opt.bank = 0; continue;
		} else if (NeStrCmp(arg, 0, "-b", "-bnk", "-bank", NULL)) {
			opt.bank = 1; opt.weem = opt.wisp = opt.oggs = 0; continue;
		} else if (NeStrCmp(arg, 0, "-o", "-ogg", NULL)) {
			opt.oggs  = 1; opt.weem = opt.wisp = opt.bank = 0; continue;
		} else if (NeStrCmp(arg, 0, "-R", "-recurse", NULL)) {
			NeTOGGLE(opt.bankrecurse); continue;
		} else if (NeStrCmp(arg, 0, "-O", "-weem2ogg", NULL)) {
			NeTOGGLE(opt.autoogg); continue;
		} else if (NeStrCmp(arg, 0, "-oi", "-ogg-inplace", NULL)) {
			NeTOGGLE(opt.ogginplace); continue;
		} else if (NeStrCmp(arg, 0, "-r", "-revorb", NULL)) {
			NeTOGGLE(opt.autorvb); continue;
		} else if (NeStrCmp(arg, 0, "-ri", "-rvb-inplace", NULL)) {
			NeTOGGLE(opt.rvbinplace); continue;
		} else if (NeStrCmp(arg, 0, "-d", "-debug", NULL)) {
			NeTOGGLE(opt.logdebug); continue;
		} else if (NeStrCmp(arg, 0, "-c", "-color", NULL)) {
			NeTOGGLE(opt.logcolor); continue;
		} else if (NeStrCmp(arg, 0, "-q", "-quiet", NULL)) {
			opt.loglevel = opt.loglevel - 1 < 0 ? 0 : opt.loglevel - 1; continue;
		} else if (NeStrCmp(arg, 0, "-Q", "-qq", "-too-quiet", NULL)) {
			NeTOGGLE(opt.logoff); continue;
		} else if (NeStrCmp(arg, 0, "-n", "-dry", "-dryrun", NULL)) {
			NeTOGGLE(opt.dryrun); continue;
		} else if (NeStrCmp(arg, 0, "-reset", NULL)) {
			NeTOGGLE(rst); continue;
		} else if ((t = NeFindArg(args, arg))) {
			t->opt = opt; opt = def; continue;
		}

		narg.arg = NeStrShallow(arg, -1);
		narg.opt = opt;

		if (narg.arg.length > args.maxarg)
			args.maxarg = narg.arg.length;
		args.argcount++;
		args.args = NeSafeAlloc(args.args, args.argcount * sizeof(struct NeArg), 0);
		args.args[args.argcount - 1] = narg;
		if (rst)
			opt = def;
	}
	args.argdigit = NeDigitCount(args.argcount);

	/* Process files */
	if (args.argcount) {
		struct NeArg *arg = &args.args[0];
		struct NeFileStat stat;
		struct NeFile afile;
		for (int i = 0; i < args.argcount; ++i, arg = &args.args[i]) {
			brrlogctl_debugon = arg->opt.logdebug;
			brrlogctl_styleon = arg->opt.logcolor;
			brrlog_setmaxpriority(arg->opt.logoff?brrlog_priority_none:arg->opt.loglevel);
			NeFileStat(&stat, NULL, arg->arg.cstr);
			if (!stat.exist || !stat.isreg || (!stat.canwt && (arg->opt.ogginplace || arg->opt.rvbinplace)) || !stat.canrd) {
				BRRLOG_WARN("Cannot parse ");
				BRRLOG_MESSAGE_STNP(brrlog_format_last.level, brrlog_color_cyan, -1, brrlog_style_bold, "%s", arg->arg.cstr);
				BRRLOG_MESSAGE_EMNP(brrlog_format_last.level, " : ");
				if (!stat.exist)
					BRRLOG_MESSAGE_STP(brrlog_format_last.level, brrlog_color_green, -1, brrlog_style_bold, "File does not exist.");
				else if (!stat.isreg)
					BRRLOG_MESSAGE_STP(brrlog_format_last.level, brrlog_color_blue, -1, brrlog_style_bold, "File is not regular.");
				else if (!stat.canwt)
					BRRLOG_MESSAGE_STP(brrlog_format_last.level, brrlog_color_green, -1, brrlog_style_bold, "Cannot write to file for in-place convert/revorb.");
				else if (!stat.canrd)
					BRRLOG_MESSAGE_STP(brrlog_format_last.level, brrlog_color_blue, -1, brrlog_style_bold, "Cannot read file.");
				continue;
			} else if (NeFileOpen(&afile, arg->arg.cstr, NeFileModeReadWrite) != NeERGNONE) {
				BRRLOG_ERRN("Could not open ");
				BRRLOG_MESSAGE_STNP(brrlog_format_last.level, brrlog_color_cyan, -1, brrlog_style_bold, "%s", arg->arg.cstr);
				BRRLOG_ERRP(" for parsing.");
				continue;
			}
			BRRLOG_NORN("Parsing ");
			BRRLOG_MESSAGE_FGNP(brrlog_format_last.level, brrlog_color_magenta, "%*i / %*i ", args.argdigit, i + 1, args.argdigit, args.argcount);
			if (arg->opt.logdebug)
				NePrintArg(*arg, args.maxarg, arg->opt.logoff);
			if ((arg->opt.oggs | arg->opt.weem | arg->opt.wisp | arg->opt.bank) == 0) {
				NeDetectType(arg, &afile);
			}
			if (arg->opt.oggs) {
				BRRLOG_MESSAGE_FGP(brrlog_format_last.level, brrlog_color_blue, "%-*s", args.maxarg, arg->arg.cstr);
				NeRevorbOgg(*arg, &afile);
			} else if (arg->opt.weem) {
				BRRLOG_MESSAGE_FGP(brrlog_format_last.level, brrlog_color_green, "%-*s", args.maxarg, arg->arg.cstr);
				NeConvertWeem(*arg, &afile);
			} else if (arg->opt.wisp) {
				BRRLOG_MESSAGE_FGP(brrlog_format_last.level, brrlog_color_yellow, "%-*s", args.maxarg, arg->arg.cstr);
				NeExtractWisp(*arg, &afile);
			} else if (arg->opt.bank) {
				BRRLOG_MESSAGE_FGP(brrlog_format_last.level, brrlog_color_red, "%-*s", args.maxarg, arg->arg.cstr);
				NeExtractBank(*arg, &afile);
			} else {
				BRRLOG_ERRN("Could not determine filetype for ");
				BRRLOG_MESSAGE_FGNP(brrlog_format_last.level, brrlog_color_cyan, "%s", arg->arg);
				BRRLOG_ERRP(", cannot parse");
			}
			if (NeFileClose(&afile) != NeERGNONE) {
				BRRDEBUG_TRACE("AAA");
			}
		}
	} else {
		brrlog_setmaxpriority(opt.logdebug ? brrlog_priority_debug : opt.loglevel);
		BRRLOG_ERR("No files passed");
	}
	args.args = NeSafeAlloc(args.args, 0, 0);

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
#endif
