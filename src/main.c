#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <alloca.h>

#include "common/NeDebugging.h"
#include "common/NeLibrary.h"
#include "common/NeMisc.h"
#include "common/NeStr.h"
#include "wisp/NeWisp.h"

static const char *const usg =
"Usage: NAeP [[OPTIONS] FILE]..."
"\n Options:"
"\n     Options take affect on all files placed1after, and can be overwritten per file."
"\n         -h, -help, -v   Print this help."
"\n         -q, -quiet      Turn off superfluous logging."
"\n         -W, -wisp       File is a wisp (.wsp) file; extract all weems from it."
"\n         -w, -weem       File is a weem (.wem) file; convert to ogg."
"\n         -b, -bank       File is a bank (.bnk) file; extract all weems found in it."
"\n         -R, -recurse    Recursively extract all weems referenced in each bank file,"
"\n                             if they're found in the given bank files."
"\n         -o, -ogg        Automatically convert extracted weem files into ogg/vorbis."
"\n         -r, -revorb     Automatically revorb converted ogg files."
;
static void printhelp()
{
	NeNORMAL("NAeP - NieR: Automata extraction Protocol");
	NeNORMAL("Compiled on %s %s", __DATE__, __TIME__);
	fprintf(stdout, "%s\n", usg);
	exit(0);
}

struct NeArgs {
	struct NeArg {
		struct NeStr arg; /* actually file to process */
		union {
			struct NeArgOpt {
				NeBy quiet:1;
				NeBy wisp:1;
				NeBy weem:1;
				NeBy bank:1;
				NeBy recurse:1;
				NeBy ogg:1;
				NeBy revorb:1;
				NeBy unused2:1;
			} options;
			NeBy opt;
		};
	} *list;
	int argcount;
};

static struct NeArg *getargbyarg(struct NeArgs args, const char *const arg) {
	for (int i = 0; i < args.argcount; ++i) {
		if (NeStrCmp(arg, 0, args.list[i].arg.cstr, NULL)) {
			return &args.list[i];
		}
	}
	return NULL;
}
static const char *getoptstr(struct NeArgOpt opt) {
	if (opt.wisp)
		return "Wisp";
	else if (opt.weem)
		return "Weem";
	else if (opt.bank)
		return "Bank";
	else
		return "UNKNOWN";
}
static void printarg(struct NeArg a, int pad) {
	NeNORMAL("%s arg '%s'%*s with %-7s logging, %-6s recursion, %-7s ogging, and %-9s revorbing.",
			getoptstr(a.options), a.arg.cstr, a.arg.length - pad, "", a.options.quiet ? "quiet" : "",
			a.options.recurse ? "BANK" : "NO", a.options.ogg ? "AUTO" : "NO",
			a.options.revorb ? "INPLACE" : "NO");
}

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

/* How work:
 * First, process all cmd arguments and build list of files, each with different,
 * specific options. Then go through the list and yada yada, do the extractions/conversions.
 * */

#if 1
#define MAXFILES 1024
int main(int argc, char **argv)
{
	struct NeArgOpt opt = {0};
	struct NeArgs args = {0};
	NeSz maxarg = 0;
	char *arg = argv[1];
	opt.wisp = 1;
	if (argc == 1)
		printhelp();

	for (int i = 1; i < argc; ++i, arg = argv[i]) {
		struct NeArg a = {0};
		if (NeStrCmp(arg, 1, "-v", "-version", "--version", "-h", "-help", "--help", NULL)) {
			printhelp();
		} else if (NeStrCmp(arg, 0, "-q", "-quiet", NULL)) {
			opt.quiet = !opt.quiet; continue;
		} else if (NeStrCmp(arg, 0, "-W", "-wisp", NULL)) {
			opt.wisp = 1;
			opt.weem = 0;
			opt.bank = 0;
			continue;
		} else if (NeStrCmp(arg, 0, "-w", "-weem", NULL)) {
			opt.wisp = 0;
			opt.weem = 1;
			opt.bank = 0;
			continue;
		} else if (NeStrCmp(arg, 0, "-b", "-bank", NULL)) {
			opt.wisp = 0;
			opt.weem = 0;
			opt.bank = 1;
			continue;
		} else if (NeStrCmp(arg, 0, "-R", "-recurse", NULL)) {
			opt.recurse = !opt.recurse; continue;
		} else if (NeStrCmp(arg, 0, "-o", "-ogg", NULL)) {
			opt.ogg = !opt.ogg; continue;
		} else if (NeStrCmp(arg, 0, "-r", "-revorb", NULL)) {
			opt.revorb = !opt.revorb; continue;
		}

		{ /* If file previously passed, update settings */
			struct NeArg *t;
			if ((t = getargbyarg(args, arg))) {
				t->options = opt;
				continue;
			}
		}

		NeStrNew(&a.arg, arg, -1);
		a.options = opt;
		if (args.argcount > MAXFILES) {
			NeERROR("WOah there, too many files!");
			break;
		}
		if (a.arg.length > maxarg)
			maxarg = a.arg.length;
		args.argcount++;
		args.list = NeSafeAlloc(args.list, args.argcount * sizeof(struct NeArg), 0);
		args.list[args.argcount - 1] = a;
	}
	if (args.argcount) {
		for (int i = 0; i < args.argcount; ++i) {
			struct NeArg a = args.list[i];
			/* with these, build list of ogg files to extract? */
			if (a.options.weem) {
				 /* extract ogg */
			} else if (a.options.wisp) {
				/* do wisp extract */
			} else if (a.options.bank) {
				/* do bank extract */
				if (a.options.recurse) {
					/* yada yada */
				}
			} else {
				NeTRACE("FUCK");
			}
			printarg(a, maxarg);
		}
	} else {
		if (!opt.quiet) {
			NeERROR("No files passed");
			printhelp();
		}
	}

	return 0;
}
#else
int main(int argc, char **argv)
{
	struct NeWisp *wisps = NULL;
	/* template path, out path, wisp path */
	struct NeStr tp = {0}, op = {0}, *wp;
	NeSz wemcount = 0;
	NeSz count = 0;
	for (int i = 1; i < argc; ++i) {
		struct NeWisp w = {0};
		if (NeWispOpen(&w, NeStrShallow(argv[i], -1)) == 0) {
			count++;
			wisps = NeSafeAlloc(wisps, count * sizeof(*wisps), 0);
			wisps[count - 1] = w;
		}
	}

	NeNORMAL("Opened %i files", count);
	for (int i = 0, err = 0; i < count && !err; ++i) {
		struct NeWisp wsp = wisps[i];
		NeSz dgt = NeDigitCount(wsp.wemCount);
		void *blk = NeSafeAlloc(NULL, NeBLOCKSIZE, 1);
		wp = &wsp.file.ppp;
		NeStrSlice(&tp, *wp, 0, NeStrRindex(*wp, NeStrShallow(".", 1), wp->length));
		for (int j = 0; j < wsp.wemCount; ++j) {
			struct NeWem wem = wsp.wems[j];
			struct NeFile output = {0};
			NeSz wrt = 0;
			NeStrPrint(&op, 0, NeMAXPATH - 4, "%s_%0*u", tp.cstr, dgt, j);
			NeStrPrint(&op, op.length, NeMAXPATH, "%s", ".wem");
			/* time to write */
			if (NeFileOpen(&output, op, NeModeWrite) != 0) {
				NeERROR("Could not open wem for writing");
				continue;
			}
			while (wrt < wem.size + 8) {
				NeSz rd = NeFileSegment(&wsp.file, blk, NeBLOCKSIZE, wem.offset + wrt, wem.offset + wem.size + 8);
				if (rd == -1) {
					NeFileClose(&output);
					err = 1;
					break;
				}
				wrt += NeFileWrite(&output, blk, rd);
			}
			NeFileClose(&output);
			wemcount++;
			if (!err)
				NeNORMAL("Extract %2zu/%-2zu %2zu/%-2zu %s", i+1, count, j+1, wsp.wemCount, op.cstr);
		}
		free(blk);
		NeWispClose(&wisps[i]);
	}
	NeStrDel(&tp);
	NeStrDel(&op);
	if (count)
		NeNORMAL("Extracted %zu weem%s from %zu wisp%s", wemcount, wemcount>1?"s":"", count, count>1?"s":"");
	wisps = NeSafeAlloc(wisps, 0, 0);
	return 0;
}
#endif
