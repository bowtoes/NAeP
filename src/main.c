#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <alloca.h>

#include "common/NeDebugging.h"
#include "common/NeLogging.h"
#include "common/NeLibrary.h"
#include "common/NeMisc.h"
#include "common/NeStr.h"
#include "wisp/NeWisp.h"

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
	NeNORMAL("NAeP - NieR: Automata extraction Protocol");
	NeNORMAL("Compiled on "__DATE__", " __TIME__"\n");
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
	NeCt maxargs = 1024;

	def.loglevel = NePrAll;
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
		opt = def; /* only for testing */
	}
	args.argdigit = NeDigitCount(args.argcount);

	/* Process files */
	if (args.argcount) {
		struct NeArg *arg = &args.args[0];
		struct NeFileStat stat;
		struct NeFile afile;
		NeLogLevelSet(NePrDebug);
		for (int i = 0; i < args.argcount; ++i, arg = &args.args[i]) {
			NeLogLevelSet(arg->opt.logdebug?NePrDebug:arg->opt.loglevel);
			NeLogColorState(arg->opt.logcolor);
			NeFileStat(&stat, NULL, arg->arg.cstr);
			if (!stat.exist || !stat.isreg || (!stat.canwt && (arg->opt.ogginplace || arg->opt.rvbinplace)) || !stat.canrd) {
				NeWARNINGN("Cannot parse ");
				NePREFIXNST(-1, NeClCyan, -1, NeStBold, "%s", arg->arg.cstr);
				NePREFIXN(-1, " : ");
				if (!stat.exist)
					NePREFIXST(-1, NeClGreen, -1, NeStBold, "File does not exist.");
				else if (!stat.isreg)
					NePREFIXST(-1, NeClBlue, -1, NeStBold, "File is not regular.");
				else if (!stat.canwt)
					NePREFIXST(-1, NeClGreen, -1, NeStBold, "Cannot write to file for in-place convert/revorb.");
				else if (!stat.canrd)
					NePREFIXST(NePrWarning, NeClGreen, -1, NeStBold, "Cannot read file.");
				continue;
			}
			if (NeFileOpen(&afile, arg->arg.cstr, NeFileModeReadWrite) != NeERGNONE) {
				NeERRORN("Could not open ");
				NePREFIXNST(NePrError, NeClCyan, -1, NeStBold, "%s", arg->arg.cstr);
				NePREFIXN(NePrError, " for parsing.");
				continue;
			}
			NeNORMALN("Parsing ");
			NePREFIXNFG(NePrNormal, NeClMagenta, "%*i / %*i ", args.argdigit, i + 1, args.argdigit, args.argcount);
			NePrintArg(*arg, args.maxarg);
			if ((arg->opt.oggs | arg->opt.weem | arg->opt.wisp | arg->opt.bank) == 0) {
				NeDetectType(arg, &afile);
			}
			if (arg->opt.oggs) {
				NePREFIXFG(NePrNormal, NeClBlue, "%-*s", args.maxarg, arg->arg.cstr);
				NeRevorbOgg(*arg, &afile);
			} else if (arg->opt.weem) {
				NePREFIXFG(NePrNormal, NeClGreen, "%-*s", args.maxarg, arg->arg.cstr);
				NeConvertWeem(*arg, &afile);
			} else if (arg->opt.wisp) {
				NePREFIXFG(NePrNormal, NeClYellow, "%-*s", args.maxarg, arg->arg.cstr);
				NeExtractWisp(*arg, &afile);
			} else if (arg->opt.bank) {
				NePREFIXFG(NePrNormal, NeClRed, "%-*s", args.maxarg, arg->arg.cstr);
				NeExtractBank(*arg, &afile);
			} else {
				NeERRORN("Could not determine filetype for ");
				NePREFIXNFG(NePrError, NeClCyan, "%s", arg->arg);
				NePREFIX(NePrError, ", cannot parse");
			}
			if (NeFileClose(&afile) != NeERGNONE) {
				NeTRACE("AAA");
			}
		}
	} else {
		NeLogLevelSet(opt.logdebug ? NePrDebug : opt.loglevel);
		NeERROR("No files passed");
	}
	args.args = NeSafeAlloc(args.args, 0, 0);

	return 0;
}

#if 0
static int extractwisp(struct NeArg a, struct NeWeem **dst) {
	struct NeWeem *d = *dst;
	struct NeWisp wsp = {0};
	struct NeStr wispPath, tempPath = {0};
	NeSz dgt = 0;
	int wemcount = 0, err = 0;

	if (!NeWispOpen(&wsp, a.arg)) {
		NeERROR("Failed to open %s for wisp extraction");
		dst = NULL;
		return 0;

	}
	wispPath = wsp.file.ppp;
	NeStrSlice(&tempPath, wispPath, 0, NeStrRindex(wispPath, NeStrShallow(".", 1), wispPath.length));

	d = NeSafeAlloc(d, wsp.wemCount * sizeof(*d), 1);
	for (int i = 0; i < wsp.wemCount; ++i) {
		struct NeWem wem = wsp.wems[i];
		struct NeWeem w = {0};
		NeSz wrt = 0;

		NeStrPrint(&w.outpath, 0, NeMAXPATH - 4, "%s_%0*u", tempPath.cstr, dgt, i);
		NeStrPrint(&w.outpath, w.outpath.length, NeMAXPATH, "%s", ".wem");
		w.size = wem.size;

		while (wrt < wem.size + 8) {
			NeSz rd = 0;
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
